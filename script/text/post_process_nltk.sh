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
  usage_pre ${0} "<models-dir>" "    <models-dir> - The location of the true-caser model files,\n                   optional, default is '.'"
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

#Set the models folder to the present dir if it is not defined
export MODELS_DIR="."
if ! [ -z "${4}" ]; then
    MODELS_DIR=${4}
fi

#Run the post-processing script
python ${BASEDIR}/post_process_nltk.py -c -t -l ${LANGUAGE} -m ${MODELS_DIR} ${INPUT_FILE} ${OUTPUT_FILE}
rc=$?
if [[ $rc != 0 ]]; then
    echo `cat ${OUTPUT_FILE}`
    exit $rc;
fi

#Output the "detected" language
echo ${LANGUAGE}
