#!/bin/bash

IO_FILE_DIR=/data/
INPUT_SUFFIX=_input
OUTPUT_SUFFIX=_output
BASH="/bin/bash -c"

command=$1
full_args=( "$*" )

echo "Args are: ${full_args}"

remaining_args=( "${@:2}" )

echo "Got command ${command}"
echo "Remaining args are ${remaining_args}"

if [ "${command}" == "run" ]
then
    set -x
    label=${remaining_args[0]}
    remaining_args=("${remaining_args[@]:1}")
    outfile_name=${label}${OUTPUT_SUFFIX}
    infile_name=${label}${INPUT_SUFFIX}
    rm -rf ${IO_FILE_DIR}/${infile_name} ${IO_FILE_DIR}/${outfile_name}
    touch ${IO_FILE_DIR}/${infile_name} ${IO_FILE_DIR}/${outfile_name}
    (tail -f ${IO_FILE_DIR}/${infile_name} | ${BASH} "${remaining_args}" 2>&1 > ${IO_FILE_DIR}/${outfile_name} &)
    set +x
elif [ "${command}" == "readfrom" ]
then
    label=${remaining_args[0]}
    outfile_name=${label}${OUTPUT_SUFFIX}
    cat ${outfile_name}
elif [ "${command}" == "writeto" ]
then
    label=${remaining_args[0]}
    remaining_args=("${remaining_args[@]:1}")
    outfile_name=${label}${OUTPUT_SUFFIX}
    infile_name=${label}${INPUT_SUFFIX}
    echo "${remainin_args}" >> ${outfile_name}
elif [ "${command}" == "raw" ]
then
     ${BASH} "${remaining_args}"
else
    echo "Could not recognize command ${command}. Letting shell handle \"$@\""
    $@ 2>&1
fi