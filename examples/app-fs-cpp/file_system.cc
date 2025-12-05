#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include "fs_app.h"
#include <vector>
#include <cstdint>
#include <iostream>


namespace fs = std::filesystem;

static std::string make_filename(const char* dir,  DataFile file, int seq)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s/%s_seq%d%s",
             dir, file.basename, seq, file.extension);
    return std::string(buf);
}

/****************************************************************
 * Decide which seq file (1 or 2) to use based on size
 ****************************************************************/
static int detect_seq_to_write(const char* dir, DataFile file, uint16_t len)
{
    std::string f1 = make_filename(dir, file, 1);
    std::string f2 = make_filename(dir, file, 2);

    bool e1 = fs::exists(f1);
    bool e2 = fs::exists(f2);

    size_t max = file.maxsize;

    // ---------------------------
    // CASE 1 — Both files exist
    // ---------------------------
    if (e1 && e2)
    {
        size_t s1 = fs::file_size(f1);
        size_t s2 = fs::file_size(f2);
        printf("both exist: s1=%zu s2=%zu max=%zu\n", s1, s2, max);

        // Determine which file is newer
        std::error_code ec1, ec2;
        auto t1 = fs::last_write_time(f1, ec1);
        auto t2 = fs::last_write_time(f2, ec2);

        int newest = (t1 > t2) ? 1 : 2;
        int oldest = (newest == 1 ? 2 : 1);

        std::string newestFile = (newest == 1 ? f1 : f2);
        size_t newestSize = (newest == 1 ? s1 : s2);

        // If newest file has enough space → write there
        if (newestSize + len <= max)
        {
            return newest;
        }

        // Otherwise, delete the oldest and write to it
        std::string oldestFile = (oldest == 1 ? f1 : f2);
        fs::remove(oldestFile);
        printf("removed oldest file %s\n", oldestFile.c_str());

        return oldest;
    }

    // ---------------------------
    // CASE 2 — Only seq1 exists
    // ---------------------------
    if (e1)
    {
        size_t s1 = fs::file_size(f1);
        if (s1 + len <= max) return 1;
        return 2;  // seq1 full → start seq2
    }

    // ---------------------------
    // CASE 3 — No files exist
    // ---------------------------
    return 1;
}

size_t read_from_file(const char* dir, DataFile file, size_t len)
{
    using namespace std;

    string f1 = make_filename(dir, file, 1);
    string f2 = make_filename(dir, file, 2);

    bool e1 = fs::exists(f1);
    bool e2 = fs::exists(f2);

    cout << "Found: " << f1 << "\n";
    cout << "Found: " << f2 << "\n";

    size_t read_bytes = 0;

    // New output file (seq0)
    string outFile = make_filename(STAGE_FILE_DOWNLOAD_DIR, file, 0);
    cout << "Writing new combined data to: " << outFile << "\n";

    ofstream ofs(outFile, ios::binary | ios::trunc);
    if (!ofs.is_open())
        return -1;

    // =====================================================================
    // Single copy routine: read from OFFSET -> END of file and write to ofs
    // =====================================================================
    auto copy_offset_to_end = [&](const string& path, size_t offset) -> size_t {
        if (!fs::exists(path)) return 0;

        size_t size = fs::file_size(path);
        if (size == 0 || offset >= size) return 0;

        size_t toRead = size - offset;
        vector<uint8_t> buf(toRead);

        cout << "Reading " << toRead <<" bytes \n";

        ifstream ifs(path, ios::binary);
        if (!ifs.is_open()) return 0;

        ifs.seekg(offset, ios::beg);
        ifs.read((char*)buf.data(), toRead);

        ofs.write((char*)buf.data(), toRead);

        ifs.close();
        return toRead;
    };

    // =====================================================================
    // CASE 1: Both seq1 & seq2 exist
    // =====================================================================
    if (e1 && e2)
    {
        // determine newest / oldest by mtime (safe with error_code)
        error_code ec1, ec2;
        auto t1 = fs::last_write_time(f1, ec1);
        auto t2 = fs::last_write_time(f2, ec2);

        string newest = (t1 > t2 ? f1 : f2);
        string oldest = (newest == f1 ? f2 : f1);

        size_t newestSize = fs::file_size(newest);
        size_t oldestSize = fs::file_size(oldest);

        cout << "New File " << newest <<" has " << newestSize << "bytes\n";

        // how many bytes we need from oldest to reach len when combined with newest
        size_t needOld = 0;
        if (newestSize < len)
            needOld = len - newestSize;

        // clamp needOld to what's actually available in oldest
        if (needOld > oldestSize)
            needOld = oldestSize;

        // compute offset into oldest: read from (oldestSize - needOld) -> end
        size_t oldestOffset = (needOld == 0) ? oldestSize : (oldestSize - needOld);
        if (oldestOffset > oldestSize) oldestOffset = 0; // safety

        cout << "Reading " << needOld <<" bytes from old datfile " << oldest << "\n";

        // read ORDER: oldest (from offset) -> newest (from start)
        read_bytes += copy_offset_to_end(oldest, oldestOffset);
        read_bytes += copy_offset_to_end(newest, 0);

        ofs.close();
        cout << "Read " << read_bytes <<" bytes Total\n";
        return read_bytes;
    }

    // =====================================================================
    // CASE 2: Only seq1 exists → read from start to end
    // =====================================================================
    if (e1)
    {
        read_bytes = copy_offset_to_end(f1, 0);
        ofs.close();
        return read_bytes;
    }

    // =====================================================================
    // CASE 3: None exist
    // =====================================================================
    ofs.close();
    return read_bytes;
}




/****************************************************************
 * WRITE ONE CHUNK
 * Directory may change every call depending on submodule
 ****************************************************************/
void write_rotating_queue_file(const char* dir,
                                DataFile file,
                               uint8_t* data,
                               size_t len)
{
    fs::create_directories(dir);

    printf("File size %ld\n",file.maxsize);
    // Figure out seq file
    int seq = detect_seq_to_write(dir, file, len);
    printf("Seg is %d\n",seq);
    std::string fname = make_filename(dir, file, seq);

    bool exists = fs::exists(fname);

    std::ofstream ofs;

    if (!exists) {
        // new file → truncate
        ofs.open(fname, std::ios::binary | std::ios::trunc);
    } else {
        // append normally
        ofs.open(fname, std::ios::binary | std::ios::app);
    }

    if (!ofs.is_open()) {
        printf("ERROR: cannot open rotated file %s\n", fname.c_str());
        return;
    }

    ofs.write(reinterpret_cast<const char*>(data), len);
    ofs.close();
}

