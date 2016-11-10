#!/bin/sh
#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a NLTK and MTMonkey based script for post-processing "
  echo "   of a text in a given language."
  echo "   The script DOES NOT support language detection"
  usage_pre ${0}
}

#Get this script actual location to find utility scripts
SCRIPT=$(readlink "${0}")
BASEDIR=$(dirname "${SCRIPT}}")

#Include the utils
. ${BASEDIR}/process_utils.sh

#Define the script type
export SCRIPT_TYPE="post"

#Process the script parameters
. ${BASEDIR}/process_params.sh

#Run the post-processing script
python ${BASEDIR}/post_process_nltk.py -c -l ${LANGUAGE} ${INPUT_FILE} ${OUTPUT_FILE}
rc=$?
if [[ $rc != 0 ]]; then
    echo `cat ${OUTPUT_FILE}`
    exit $rc;
fi

#Output the "detected" language
echo ${LANGUAGE}
