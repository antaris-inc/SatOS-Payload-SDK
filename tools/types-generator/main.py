#
#   Copyright 2022 Antaris, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import os, sys, getopt
import cpp_generator as CPP
import proto_generator as PROTO
import py_generator as PY
import parser_interface as PARSER
import logging

# Creating a logger object
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__.split('.')[0])

gInputFile=None
gOutLanguage="cpp"
gSchemaFile=None
gOutFileBaseName=None
gOutDir=None

gValidLanguages = {"cpp", "python", "proto"}

gXML=None

def print_usage():
    logger.error("{}: Usage".format(sys.argv[0]))
    logger.error("{} -i/--input-file INPUT_FILE -o/--output-dir OUTDIR -s/--schema-file SCHEMA-FILE [-l/--language LANGUAGE (default cpp, options cpp/python)] [-h/--help]".format(sys.argv[0]))

def print_params():
    global gInputFile
    global gOutLanguage
    global gSchemaFile
    global gOutFileBaseName
    global gOutDir

    logger.info("{}: working with parameters".format(sys.argv[0]))
    logger.info("INPUT-FILE={}".format(gInputFile))
    logger.info("LANGUAGE={}".format(gOutLanguage))
    logger.info("SCHEMA={}".format(gSchemaFile))
    logger.info("OUTDIR={}".format(gOutDir))
    logger.info("BASEFILENAME={}".format(gOutFileBaseName))

def set_base_file_name():
    global gInputFile
    global gOutFileBaseName

    gOutFileBaseName = PARSER.get_file_basename_without_extension(gInputFile)

def parse_opts():
    global gInputFile
    global gOutLanguage
    global gSchemaFile
    global gValidLanguages
    global gOutDir

    argv = sys.argv[1:]
    print("Got args: {}".format(argv))
    try:
      opts, args = getopt.getopt(argv, "hi:l:o:s:",["help", "input-file=", "language=", "output-dir=", "schema-file="])
    except getopt.GetoptError:
      logger.critical ('Error parsing arguments')
      print_usage()
      sys.exit(-1)

    for opt, arg in opts:
        print("Parsing option {} with value {}".format(opt, arg))
        if opt in ('-h', "--help"):
            print_usage()
            sys.exit()
        elif opt in ("-i", "--input-file"):
            gInputFile = arg
        elif opt in ("-l", "--language"):
            gOutLanguage = arg
            if gOutLanguage not in gValidLanguages:
                logger.critical("Language {} not recognized".format(arg))
        elif opt in ("-s", "--schema-file"):
            gSchemaFile = arg
        elif opt in ("-o", "--output-dir"):
            gOutDir = arg

    if None == gInputFile:
        logger.critical("Compulsory parameter input-file missing")
        print_usage()
        sys.exit(-1)

    if None == gSchemaFile:
        logger.critical("Compulsory parameter schema-file missing")
        print_usage()
        sys.exit(-1)

    if None == gOutDir:
        logger.critical("Compulsory parameter output-dir missing")
        print_usage()
        sys.exit(-1)

    set_base_file_name()

def generate_cpp():
    global gInputFile
    global gSchemaFile
    global gXML
    global gOutDir
    global gOutFileBaseName

    outdir = "{}/cpp/gen/".format(gOutDir)

    if not os.path.exists(outdir):
        os.makedirs(outdir)

    header_file = "{}{}.h".format(outdir, gOutFileBaseName)
    source_file = "{}{}_autogen.cc".format(outdir, gOutFileBaseName)
    namespace = "::{}_peer_to_peer::".format(gOutFileBaseName)

    header_handle = open(header_file, "w")
    source_handle = open(source_file, "w")

    header_handle = open(header_file, "w")
    source_handle = open(source_file, "w")

    logger.info("Generating cpp files")

    codeGenerator = CPP.InterfaceGen(gInputFile, gSchemaFile, header_handle, source_handle, namespace)
    codeGenerator.go()

    header_handle.close()
    source_handle.close()

def generate_proto():
    global gInputFile
    global gSchemaFile
    global gXML
    global gOutDir
    global gOutFileBaseName

    outdir = "{}".format(gOutDir)

    if not os.path.exists(outdir):
        os.makedirs(outdir)

    header_file = "{}/{}.proto".format(outdir, gOutFileBaseName)

    header_handle = open(header_file, "w")

    logger.info("Generating proto file ---> {}".format(header_file))

    codeGenerator = PROTO.InterfaceGen(gInputFile, gSchemaFile, header_handle, None)
    codeGenerator.go()

    header_handle.close()

    header_handle.close()

def generate_python():
    global gInputFile
    global gSchemaFile
    global gXML
    global gOutDir
    global gOutFileBaseName

    outdir = "{}/python/satos_payload_sdk/gen/".format(gOutDir)

    if not os.path.exists(outdir):
        os.makedirs(outdir)

    header_file = "{}{}_types.py".format(outdir, gOutFileBaseName)

    header_handle = open(header_file, "w")

    logger.info("Generating python files")

    codeGenerator = PY.InterfaceGen(gInputFile, gSchemaFile, header_handle, None, gOutFileBaseName)
    codeGenerator.go()

    header_handle.close()

def generate_code():
    global gOutLanguage

    generators = {"cpp": generate_cpp, "python": generate_python, "proto": generate_proto}

    generators[gOutLanguage]()

# Main function
if __name__ == '__main__':
    parse_opts()
    print_params()
    generate_code()
