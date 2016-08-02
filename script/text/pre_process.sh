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
  echo "    <language> - the processor job language."
  echo "                 If set to \"auto\" needs to be detected"
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a dummy default script for testing the text pre-processors."
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   Perform pre-processing of the given utf-8 text. Is to be called from"
  echo "   the bpbd-processor server application. This script may perform a default"
  echo "   pre-processing for an unsupported language, if the default pre-processing"
  echo "   is not available must return an error. If the language is set to auto is"
  echo "   expected to detect the language, if the language detection is not supported"
  echo "   must return an error. If the language is detected and is supported then a"
  echo "   language-specific pre-processor must be called. If the language is detected"
  echo "   but is not supported then a default pre-processor is to be used, if possible."
  echo "   Preprocessing includes, but is not limited by: "
  echo "      * tokenization - splitting the language words and punctuation marks with spaces"
  echo "      * lowercasing  - turning the text into the lowercase"
  echo "      * unification  - unifying the text by substituting longer utf-8 symbols with shorter ones"
  echo "   The output of this script must be two fold:"
  echo "      * If the preprocessing went fine:"
  echo "         - The exit code must be 0"
  echo "         - The only data written to standard output must be the input text language"
  echo "         - The output pre-processed text must be written into:"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "      * If the preprocessing failed:"
  echo "         - The exit code must be 1"
  echo "         - The only data written to standard output must be the error message, max 1023 characters long"
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   This is a dummy script that only emulates the real behavior, therefore:"
  echo "      * The auto detected language returned by this script is always \"german\""
  echo "      * The script only copies the input file into the output."
  usage ${0}
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

#Perform error checks of the input data
if [ "$#" -eq 0 ]; then
   error ${0} "Improper number of arguments!"
   info ${0}
   fail
fi
if [ -z "${1}" ]; then
   error ${0} "<work-dir> is not defined"
   usage ${0}
   fail
fi
if [ -z "${2}" ]; then
   error ${0} "<job-uid> is not defined"
   usage ${0}
   fail
fi
if [ -z "${3}" ]; then
   error ${0} "<language> is not defined"
   usage ${0}
   fail
fi
if ! [ -e "${1}" ]; then
   error ${0} "${1} not found!"
   usage ${0}
   fail
fi
if ! [ -d "${1}" ]; then
   error ${0} "${1} is not a directory!"
   usage ${0}
   fail
fi

#Store the directory in which the processor files reside
export WORK_DIR=${1}

#Read the job id and check on it, create input output file names
export JOB_UID=${2}
export INPUT_FILE=${WORK_DIR}/${JOB_UID}.pre.in.txt
export OUTPUT_FILE=${WORK_DIR}/${JOB_UID}.pre.out.txt

#Get the language
export LANGUAGE=${3}

#Check if the language is to be auto detected
if [ "${LANGUAGE}" = "auto" ]; then
   LANGUAGE="german"
fi

#Output the "detected" language
echo ${LANGUAGE}

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

cp ${INPUT_FILE} ${INPUT_FILE}.tmp
cp ${OUTPUT_FILE} ${OUTPUT_FILE}.tmp
