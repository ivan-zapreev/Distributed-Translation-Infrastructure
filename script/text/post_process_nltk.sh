#!/bin/sh

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a NLTK and MTMonkey based script for post-processing "
  echo "   of a text in a given language. The true casing is done here"
  echo "   in two possible ways, with: "
  echo "      (i) Truecaser https://github.com/nreimers/truecaser"
  echo "      (ii) Moses https://github.com/moses-smt/mosesdecoder"
  echo "   the required scripts are included into this project, so there"
  echo "   is no need to download them again. The script DOES NOT, and "
  echo "   never will, support language detection as it is not needed."
  
  help1="    <true_caser_type> - the true casing software to use, optional:"
  help2="        'truecaser' - https://github.com/nreimers/truecaser"
  help3="        'moses' - https://github.com/moses-smt/mosesdecoder"
  help4="    <models-dir> - The location of the true-caser model files,\n"
  help5="                   optional, default is '.'"
  usage_pre ${0} "<true_caser_type> <models-dir>" "${help1}${help2}${help3}${help4}${help5}"
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

#Check if the true caser is even requested
if [ -z "${4}" ]; then
    #The true caser is not requested, also no capitalization
    python ${BASEDIR}/post_process_nltk.py -l ${LANGUAGE} -m ${MODELS_DIR} ${INPUT_FILE} ${OUTPUT_FILE}
else
    #Set the models folder to the present dir if it is not defined
    export MODELS_DIR="."
    if ! [ -z "${5}" ]; then
        MODELS_DIR=${5}
    fi
    
    #Check on the truecaser type, since it is present
    case "${4}" in
        truecaser)
            #Run the post-processing script with truecaser
            python ${BASEDIR}/post_process_nltk.py -c -t -l ${LANGUAGE} -m ${MODELS_DIR} ${INPUT_FILE} ${OUTPUT_FILE}
        ;;
        moses)
        ;;
        *)
        echo "Unrecognized truecaser option: '${4}', needs one of: {truecaser, moses}"
        exit 1
    esac
fi

#Check on the scripts' result
rc=$?
if [[ $rc != 0 ]]; then
    echo `cat ${OUTPUT_FILE}`
    exit $rc;
fi

#Output the "detected" language
echo ${LANGUAGE}
