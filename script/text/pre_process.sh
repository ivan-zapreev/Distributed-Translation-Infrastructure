#!/bin/sh

#Prints the program usage
# ${0} - the program name
function usage(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <work-dir> <job-uid> <language>"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the pre-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.pre.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.pre.out.txt"
  echo "    <language> - the pre-processor input text language."
  echo "                 If set to \"auto\", the language is to be detected."
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a dummy script for pre-processing of a text in a given language."
  usage ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   Perform pre-processing of the given utf-8 text. Is to be called from"
  echo "   the bpbd-processor server application. The script must work as follows:"
  echo "      1. if <language> == \"auto\":"
  echo "        1.1. True: if language detection is enabled"
  echo "           1.1.1. True: detect a language:"
  echo "              1.1.1.1. Detected: remember the source language"
  echo "              1.1.1.2. Failed: report an error to the standard output and finish"
  echo "           1.1.2. False: report an error to the standard output and finish"
  echo "        1.2. False: nothing"
  echo "      2. Check if the language pre-processor is available"
  echo "        2.1. Available: call the language pre-processor"
  echo "           2.1. Success: output the language string to the standard output"
  echo "           2.1. Fail: report an error to the standard output and finish"
  echo "        2.1. Not available: check if the default pre-processor is available"
  echo "           2.1.1. Available: call the default pre-processor"
  echo "              2.1.1.1. Success: output the language string to the standard output"
  echo "              2.1.1.2. Fail: report an error to the standard output and finish"
  echo "           2.1.2. Not available: report an error to the standard output and finish"
  echo "   Pre-processing might include, but is not limited by: "
  echo "      * language detection - detecting the language in which the text is written"
  echo "      * tokenization - splitting the language words and punctuation marks with spaces"
  echo "      * lowercasing  - turning the text into the lowercase"
  echo "      * unification  - unifying the text by substituting longer utf-8 symbols with shorter ones"
  echo "   In addition to mentioned above, the result of this script must be two fold:"
  echo "      * If the pre-processing went without errors:"
  echo "         - The exit code must be 0"
  echo "         - The only data written to standard output must be the <language>"
  echo "         - The pre-processed text must be written into the file:"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "      * If the pre-processing failed:"
  echo "         - The exit code must be 1"
  echo "         - The only data written to standard output must be the error message, max 1023 characters long"
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   * The bpbd-processor application after consuming the file:"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "     will attempt to delete both pre-processing job files:"
  echo "              <work-dir>/<job-uid>.pre.in.txt"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "     So, if these files are to be kept for, e.g. post-processing,"
  echo "     it is the responsibility of the script to do so."
  echo "   * This is a dummy script that only emulates the real behavior, therefore:"
  echo "      * The script only detects the input language to be \"german\""
  echo "      * The script only copies the input file into the output"
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
export INPUT_FILE=${WORK_DIR}/${JOB_UID}.pre.in.txt
export OUTPUT_FILE=${WORK_DIR}/${JOB_UID}.pre.out.txt

if ! [ -e "${INPUT_FILE}" ]; then
   error "${INPUT_FILE} could not be found!"
   fail
fi

#Get the language
export LANGUAGE=${3}

#Check if the language is to be auto detected
if [ "${LANGUAGE}" = "auto" ]; then
   LANGUAGE="german";
fi

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

#Output the "detected" language
echo ${LANGUAGE}
