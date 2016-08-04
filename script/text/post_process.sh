#!/bin/sh

#Prints the program usage
# ${0} - the program name
function usage(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <work-dir> <job-uid> <language>"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the post-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.post.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.post.out.txt"
  echo "    <language> - the post-processor input text language."
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a dummy script for post-processing of a text in a given language."
  usage ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   Perform post-processing of the given utf-8 text. Is to be called from"
  echo "   the bpbd-processor server application. The script must work as follows:"
  echo "      1. if <language> == \"auto\":"
  echo "        1.1. True: report an error to the standard output and finish"
  echo "        1.2. False: nothing"
  echo "      2. Check if the language post-processor is available"
  echo "        2.1. Available: call the language post-processor"
  echo "           2.1. Success: output the language string to the standard output"
  echo "           2.1. Fail: report an error to the standard output and finish"
  echo "        2.1. Not available: check if the default post-processor is available"
  echo "           2.1.1. Available: call the default post-processor"
  echo "              2.1.1.1. Success: output the language string to the standard output"
  echo "              2.1.1.2. Fail: report an error to the standard output and finish"
  echo "           2.1.2. Not available: report an error to the standard output and finish"
  echo "   Post-processing includes, but is not limited by: "
  echo "      * de-tokenization - removing unnecessry whitespaces"
  echo "      * de-lowercasing  - restoring the capital letters in the wext where needed"
  echo "      * restoring structure - making the translated text to have structure similar"
  echo "                              to that of the original. Note that the script might,"
  echo "                              does not have to, be able to find the original source"
  echo "                              texts under the names:"
  echo "              				     <work-dir>/<job-uid>.pre.in.txt"
  echo "              				     <work-dir>/<job-uid>.pre.out.txt"
  echo "              				  This is if the same preprocessing server was used to"
  echo "              				  perform the source text pre- processing."
  echo "   In addition to mentioned above, the output of this script must be two fold:"
  echo "      * If the post-processing went without errors:"
  echo "         - The exit code must be 0"
  echo "         - The only data written to standard output must be the <language>"
  echo "         - The post-processed text must be written into the file:"
  echo "              <work-dir>/<job-uid>.post.out.txt"
  echo "      * If the post-processing failed:"
  echo "         - The exit code must be 1"
  echo "         - The only data written to standard output must be the error message,"
  echo "           max 1023 characters long"
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   This is a dummy script that only emulates the real behavior, therefore:"
  echo "      * The script only copies the input file into the output."
}

#Reports an error, does not exit
# ${0} - the script name
# ${1} - the error message
function error() {
   echo "ERROR in ${0}: ${1}"
}

#Allows the program to fail
function fail() {
   exit 1
}

#If the script is called with no arguments then show the info message
if [ "$#" -eq 0 ]; then
   error "Improper number of arguments!"
   info ${0}
   fail
fi

#Check if the work directory param is defined and the directory is present
if [ -z "${1}" ]; then
   error "<work-dir> is not defined"
   fail
fi
if ! [ -e "${1}" ]; then
   error "${1} not found!"
   fail
fi
if ! [ -d "${1}" ]; then
   error "${1} is not a directory!"
   fail
fi

#Check if the job uid param is defined
if [ -z "${2}" ]; then
   error "<job-uid> is not defined"
   fail
fi

#Check if the language param is defined
if [ -z "${3}" ]; then
   error "<language> is not defined"
   fail
fi

#Store the directory in which the processor files reside
export WORK_DIR=${1}

#Read the job id and check on it, create input output file names
export JOB_UID=${2}
export INPUT_FILE=${WORK_DIR}/${JOB_UID}.post.in.txt
export OUTPUT_FILE=${WORK_DIR}/${JOB_UID}.post.out.txt

if ! [ -e "${INPUT_FILE}" ]; then
   error "${INPUT_FILE} could not be found!"
   fail
fi

#Get the language
export LANGUAGE=${3}

#Check if the language is to be auto detected
if [ "${LANGUAGE}" = "auto" ]; then
   error "The language auto detection is not allowed!"
   fail
fi

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

#Output the "detected" language
echo ${LANGUAGE}
