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
  
  help1="    <true_caser_type> - the true casing software to use:"
  help2="        'none' - there will be no truecasing done"
  help3="        'truecaser' - https://github.com/nreimers/truecaser"
  help4="        'moses' - https://github.com/moses-smt/mosesdecoder"
  help5="    <models-dir> - The location of the true-caser model files,\n"
  help6="                   optional, default is '.'"
  usage_pre ${0} "<true_caser_type> <models-dir>" "${help1}${help2}${help3}${help4}${help5}${help6}"
}

#Clean up after the script before existing
function clean() {
    #Remove the tmp file
    rm -f ${INTERM_FILE}
    #Remove the template file
    rm -f ${TEMPL_FILE}
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
    error "The truecaser option is not defined!"
    fail
else
    #Set the models folder to the present dir if it is not defined
    export MODELS_DIR="."
    if ! [ -z "${5}" ]; then
        MODELS_DIR=${5}
    fi
    
    #Extract the truecaser type
    TRUE_CASE_TYPE=${4}
    
    #Check on the truecaser type, since it is present
    case "${TRUE_CASE_TYPE}" in
        none)
            #Run the post-processing script NO truecasing
            python ${BASEDIR}/post_process_nltk.py -c -l ${LANGUAGE} -m ${MODELS_DIR} -t ${TEMPL_FILE} ${INPUT_FILE} ${OUTPUT_FILE}
        ;;
        truecaser)
            #Run the post-processing script with truecaser
            python ${BASEDIR}/post_process_nltk.py -c -u -l ${LANGUAGE} -m ${MODELS_DIR} -t ${TEMPL_FILE} ${INPUT_FILE} ${OUTPUT_FILE}
        ;;
        moses)
            #Define the intermediate output file
            INTERM_FILE=${OUTPUT_FILE}.tmp

            #Check for the model file to exist
            MODEL_FILE=${MODELS_DIR}/${LANGUAGE}.tcm
            if ! [ -e "${MODEL_FILE}" ]; then
               error "Language '${LANGUAGE}' is not supported by the truecaser!"
               fail
            fi
            
            #Add moses true casing
            perl ${BASEDIR}/truecase/moses/truecase.perl --model ${MODEL_FILE} < ${INPUT_FILE} > ${INTERM_FILE}

            #Check on the scripts' result
            rc=$?
            if [[ $rc != 0 ]]; then
                echo `cat ${OUTPUT_FILE}`
                clean
                exit $rc;
            fi
            
            #Do detokenization and capitalization
            python ${BASEDIR}/post_process_nltk.py -c -l ${LANGUAGE} -m ${MODELS_DIR} -t ${TEMPL_FILE} ${INTERM_FILE} ${OUTPUT_FILE}
            ;;
        *)
        error "Unrecognized truecaser option: '${TRUE_CASE_TYPE}'!"
        fail
    esac
fi

#Check on the scripts' result
rc=$?
if [[ $rc != 0 ]]; then
    echo `cat ${OUTPUT_FILE}`
    clean
    exit $rc;
fi

#DEBUG: Create back files for ananlysis
#cp ${INPUT_FILE} ${INPUT_FILE}.bak
#cp ${INTERM_FILE} ${INTERM_FILE}.bak
#cp ${OUTPUT_FILE} ${OUTPUT_FILE}.bak

#Clean before exiting, the intefmediate
#and template files are no longer needed
clean

#Output the "detected" language
echo ${LANGUAGE}
