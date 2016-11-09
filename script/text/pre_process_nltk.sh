#!/bin/sh

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a NLTK based script for pre-processing of a text in"
  echo "   a given language. The script supports language detection"
  usage_pre ${0}
}

#Get this script actual location to find utility scripts
SCRIPT=$(readlink "${0}")
BASEDIR=$(dirname "${SCRIPT}}")

#Include the utils
. ${BASEDIR}/process_utils.sh

#Define the script type
export SCRIPT_TYPE="pre"

#Process the script parameters
. ${BASEDIR}/process_params.sh

#Check if the language is to be auto detected
export DETECTED_LANG=""
if [ "${LANGUAGE}" = "auto" ]; then
    #Execute the language detection script there is and return an error if needed
    DETECTED_LANG=$(python ${BASEDIR}/lang_detect_nltk.py ${INPUT_FILE})
    rc=$?
    if [[ $rc != 0 ]]; then
        echo ${DETECTED_LANG}
        exit $rc;
    fi
else
    DETECTED_LANG=${LANGUAGE}
fi

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

#Output the "detected" language
echo ${DETECTED_LANG}
