#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include "fs_app.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include "antaris_api.h"

namespace fs = std::filesystem;

constexpr size_t MAX_QUEUES = 40;

struct QueueOffsetEntry
{
    uint64_t write_offset;   // where next write should go
};

static constexpr size_t MAX_QUEUE_FILE_SIZE = 1 * 1024 * 1024; // 1 MB
static constexpr const char* OFFSET_FILE = "offset.bin";


uint64_t read_queue_offset(const std::string& offsetFile, uint8_t queueId)
{
    using namespace std;

    if (!fs::exists(offsetFile))
        return 0;

    ifstream ifs(offsetFile, ios::binary);
    if (!ifs.is_open())
        return 0;

    QueueOffsetEntry entry{};
    ifs.seekg(queueId * sizeof(QueueOffsetEntry), ios::beg);
    ifs.read(reinterpret_cast<char*>(&entry), sizeof(entry));

    return entry.write_offset;
}

void write_queue_offset(const std::string& offsetFile,
                        uint8_t queueId,
                        uint64_t offset)
{
    using namespace std;

    // Ensure file exists and is sized correctly
    if (!fs::exists(offsetFile))
    {
        ofstream init(offsetFile, ios::binary | ios::trunc);
        QueueOffsetEntry zero{};
        for (size_t i = 0; i < MAX_QUEUES; i++)
            init.write(reinterpret_cast<char*>(&zero), sizeof(zero));
        init.close();
    }

    fstream fs(offsetFile, ios::binary | ios::in | ios::out);
    QueueOffsetEntry entry{offset};

    fs.seekp(queueId * sizeof(QueueOffsetEntry), ios::beg);
    fs.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    fs.close();
}

static std::string make_filename(const char* dir,  DataFile file)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s/%s%s",
             dir, file.basename, file.extension);
    return std::string(buf);
}

size_t read_rotating_queue_file(const char* dir,
                                DataFile file,
                                uint8_t queueId,
                                const std::string& outFile)
{
    using namespace std;

    string dataFile   = make_filename(dir, file);
    string offsetFile = string(dir) + "/" + OFFSET_FILE;

    if (!fs::exists(dataFile))
        return 0;

    size_t fileSize = fs::file_size(dataFile);
    if (fileSize == 0)
        return 0;

    uint64_t offset = read_queue_offset(offsetFile, queueId);

    // Safety clamp (corruption / partial write protection)
    if (offset > fileSize)
        offset = fileSize;

    ifstream ifs(dataFile, ios::binary);
    if (!ifs.is_open())
        return 0;

    ofstream ofs(outFile, ios::binary | ios::trunc);
    if (!ofs.is_open())
        return 0;

    size_t totalRead = 0;

    // ======================================================
    // CASE 1: File not wrapped yet (size < MAX)
    // Data is linear from 0 → offset
    // ======================================================
    if (fileSize < MAX_QUEUE_FILE_SIZE)
    {
        vector<uint8_t> buf(offset);

        ifs.seekg(0, ios::beg);
        ifs.read(reinterpret_cast<char*>(buf.data()), offset);

        ofs.write(reinterpret_cast<char*>(buf.data()), offset);
        totalRead = offset;
    }
    else
    {
        // ==================================================
        // CASE 2: Wrapped circular buffer
        // Oldest = offset
        // ==================================================
        size_t firstPart = fileSize - offset;

        // Read oldest → end
        vector<uint8_t> buf1(firstPart);
        ifs.seekg(offset, ios::beg);
        ifs.read(reinterpret_cast<char*>(buf1.data()), firstPart);
        ofs.write(reinterpret_cast<char*>(buf1.data()), firstPart);

        totalRead += firstPart;

        // Read newest (0 → offset)
        if (offset > 0)
        {
            vector<uint8_t> buf2(offset);
            ifs.seekg(0, ios::beg);
            ifs.read(reinterpret_cast<char*>(buf2.data()), offset);
            ofs.write(reinterpret_cast<char*>(buf2.data()), offset);

            totalRead += offset;
        }
    }

    ifs.close();
    ofs.close();

    return totalRead;
}

/****************************************************************
 * WRITE ONE CHUNK
 * Directory may change every call depending on submodule
 ****************************************************************/
AntarisReturnCode write_rotating_queue_file(const char* dir,
                               DataFile file,
                               uint8_t queueId,
                               uint8_t* data,
                               size_t len)
{
    using namespace std;

    if (queueId >= MAX_QUEUES)
        return An_NOT_PERMITTED;

    fs::create_directories(dir);

    string dataFile = make_filename(dir, file);
    string offsetFile = string(dir) + "/" + OFFSET_FILE;

    if (len > MAX_QUEUE_FILE_SIZE)
        return An_NOT_PERMITTED;

    uint64_t offset = read_queue_offset(offsetFile, queueId);
    cout << "Starting offset " << offset << endl;

    // Open data file (create if needed)
    fstream fs(dataFile, ios::binary | ios::in | ios::out);
    if (!fs.is_open())
    {
        fs.open(dataFile, ios::binary | ios::out | ios::trunc);
        fs.close();
        fs.open(dataFile, ios::binary | ios::in | ios::out);
    }

    // ----------------------------------------------------
    // Write with wrap-around
    // ----------------------------------------------------
    size_t firstChunk = min(len, MAX_QUEUE_FILE_SIZE - offset);
    size_t secondChunk = len - firstChunk;

    cout << "First Chuckk " << firstChunk  << "Second Chunk " << secondChunk << endl;

    fs.seekp(offset, ios::beg);
    fs.write(reinterpret_cast<const char*>(data), firstChunk);

    if (secondChunk > 0)
    {
        fs.seekp(0, ios::beg);
        fs.write(reinterpret_cast<const char*>(data + firstChunk), secondChunk);
    }

    fs.close();

    // Update offset
    uint64_t newOffset = (offset + len) % MAX_QUEUE_FILE_SIZE;
    cout << "End  offset " << newOffset << endl;
    write_queue_offset(offsetFile, queueId, newOffset);

    return An_SUCCESS;
}

