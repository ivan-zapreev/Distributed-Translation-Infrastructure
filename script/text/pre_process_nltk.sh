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

#Clean up after the script before existing
function clean() {
    #Remove the template file
    rm -f ${TEMPL_FILE}
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
        clean
        exit $rc;
    fi
else
    DETECTED_LANG=${LANGUAGE}
fi

#Run the pre-processing script
python ${BASEDIR}/pre_process_nltk.py -l ${DETECTED_LANG} -t ${TEMPL_FILE} ${INPUT_FILE} > ${OUTPUT_FILE}
rc=$?
if [[ $rc != 0 ]]; then
    echo `cat ${OUTPUT_FILE}`
    clean
    exit $rc;
fi

#DEBUG: Create back files for ananlysis
#cp ${INPUT_FILE} ${INPUT_FILE}.bak
#cp ${TEMPL_FILE} ${TEMPL_FILE}.bak
#cp ${OUTPUT_FILE} ${OUTPUT_FILE}.bak

#Output the "detected" language
echo ${DETECTED_LANG}
