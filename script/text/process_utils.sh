#!/bin/bash

#Prints the program usage
# ${0} - the program name
# ${1} - the function name
# ${2} - the list of additional parameters (optional)
# ${3} - the additional parameter's description (optional)
function usage_post(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " ${0} --work-dir=<work-dir> --job-uid=<job-uid> --lang=<language> ${2}"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the post-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.post.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.post.out.txt"
  echo "    <language> - the post-processor input text language."
  if ! [ -z "${3}" ]; then
    echo "${3}"
  fi
}

#Prints the program usage
# ${0} - the program name
# ${1} - the function name
# ${2} - the list of additional parameters (optional)
# ${3} - the additional parameter's description (optional)
function usage_pre(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " ${0} --work-dir=<work-dir> --job-uid=<job-uid> --lang=<language> ${2}"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the pre-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.pre.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.pre.out.txt"
  echo "    <language> - the pre-processor input text language."
  echo "                 If set to \"auto\", the language is to be detected."
  if ! [ -z "${3}" ]; then
    echo "${3}"
  fi
}

#Reports an error to stdout, does not exit
# ${0} - the script name
# ${1} - the error message
function error() {
   echo "ERROR in ${0}: ${1}"
}

#Allows the program to fail
function fail() {
   exit 1
}

#Allows to check on the exit code fiven as a
#parameter and fail calling on the clean function.
#This script also cats the output file as it shall
#then store the error message
# ${0} - the script name
# ${1} - the exit code of a process
function check_clean_fail() {
    rc=$1
    if [[ $rc != 0 ]]; then
        cat ${OUTPUT_FILE}
        clean
        exit $rc;
    fi
}
