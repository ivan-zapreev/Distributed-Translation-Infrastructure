#!/bin/bash
#
# File:   post_process_nltk.sh
# Author: Dr. Ivan S. Zapreev
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Created on November 14, 2016, 11:07 AM
#

#Prints the program info
# ${0} - the program name
function info() {
    echo "------"
    echo "SHORT:"
    echo "------"
    echo "   This is a NLTK and MTMonkey based script for post-processing "
    echo "   of a text in a given language. For de-tokenization this script"
    echo "   supports the following languages: "
    echo "      English, French, Spanish, Italian, Czech"
    echo "   The true casing is done here in two possible ways, with: "
    echo "      (i) Truecaser https://github.com/nreimers/truecaser"
    echo "      (ii) Moses https://github.com/moses-smt/mosesdecoder"
    echo "   the required scripts are included into this project, so there"
    echo "   is no need to download them again. Appropriate language models"
    echo "   are required for a truecaser to work. Also, this script DOES NOT,"
    echo "   and never will, support language detection as it is not needed."

    help1="    <true_caser_type> - the true casing software to use:\n"
    help2="        'none' - there will be no truecasing done, is default\n"
    help3="        'truecaser' - https://github.com/nreimers/truecaser\n"
    help4="        'moses' - https://github.com/moses-smt/mosesdecoder\n"
    help5="    <models-dir> - The location of the true-caser model files is optional,\n"
    help6="                   the default location is: '.' The model file name shall\n"
    help7="                   be the lower-cased language name written in English. For\n"
    help8="                   moses the model extension is '.tcm' and for truecaser it\n"
    help9="                   is '.obj'. For example: english.tcm and english.obj."
    usage_pre ${0} "--tc-type=<true_caser_type> --models-dir=<models-dir>" "${help1}${help2}${help3}${help4}${help5}${help6}${help7}${help8}${help9}"
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

#################################################################
#Define and initialize the additional script parameter values
TRUE_CASE_TYPE="none"
MODELS_DIR="."

#Parse the additional parameters of the post-processing script
for i in "$@"
do
    case $i in
        --tc-type=*)
            #Store the true caser type
            TRUE_CASE_TYPE="${i#*=}"
            shift
            ;;
        --models-dir=*)
            #Store the true caser models directory
            MODELS_DIR="${i#*=}"
            shift
            ;;
        *)
            #Do nothing, this is some other parameter to be parsed elsewhere
            shift
            ;;
    esac
done

#################################################################
#Check if the template file exists, then pass it on to the scripts
TEMPLATE_OPTION=""
if [ -e ${TEMPL_FILE} ]; then
    TEMPLATE_OPTION="-t ${TEMPL_FILE}"
fi

#################################################################
#Check on the truecaser type, since it is present
case "${TRUE_CASE_TYPE}" in
    none)
        #Run the post-processing script NO truecasing
        python ${BASEDIR}/post_process_nltk.py -c -l ${LANGUAGE} -m ${MODELS_DIR} ${TEMPLATE_OPTION} ${INPUT_FILE} ${OUTPUT_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?
    ;;
    truecaser)
        #Run the post-processing script with truecaser
        python ${BASEDIR}/post_process_nltk.py -c -u -l ${LANGUAGE} -m ${MODELS_DIR} ${TEMPLATE_OPTION} ${INPUT_FILE} ${OUTPUT_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?
    ;;
    moses)
        #Define the intermediate output file
        INTERM_FILE=${OUTPUT_FILE}.tmp

        #Lowercase the language name to get the proper file name
        LANGUAGE_LC=`echo ${LANGUAGE} | tr '[:upper:]' '[:lower:]'`

        #Check for the model file to exist
        MODEL_FILE=${MODELS_DIR}/${LANGUAGE_LC}.tcm
        if ! [ -e "${MODEL_FILE}" ]; then
           error "Language '${LANGUAGE}' is not supported by the truecaser!"
           fail
        fi

        #Add moses true casing
        perl ${BASEDIR}/truecase/moses/truecase.perl --model ${MODEL_FILE} < ${INPUT_FILE} > ${INTERM_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?

        #Do detokenization and capitalization
        python ${BASEDIR}/post_process_nltk.py -c -l ${LANGUAGE} -m ${MODELS_DIR} ${TEMPLATE_OPTION} ${INTERM_FILE} ${OUTPUT_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?
        ;;
    *)
        error "Unrecognized truecaser option: '${TRUE_CASE_TYPE}'!"
        fail
        ;;
esac

#DEBUG: Create back files for ananlysis
#cp -f ${INPUT_FILE} ${INPUT_FILE}.bak
#cp -f ${INTERM_FILE} ${INTERM_FILE}.bak
#cp -f ${OUTPUT_FILE} ${OUTPUT_FILE}.bak

#Clean before exiting, the intefmediate
#and template files are no longer needed
clean

#Output the "detected" language
echo ${LANGUAGE}
