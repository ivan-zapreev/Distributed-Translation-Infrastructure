#!/bin/sh
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
    echo "   This is a NLTK based script for pre-processing of a text in"
    echo "   a given language. The script supports language detection"

    help1="    <core-nlp-dir> - the directory with Stanford Core NLP\n"
    help2="                     as obtained from:\n"
    help3="                        http://stanfordnlp.github.io/CoreNLP\n"
    help4="                     Optional, is only needed for Chinese.\n"

    help5="    <back-ends> - the comma-separated list of back-end server(s)\n"
    help6="                   for the Stanford Core NLP, in the form:\n"
    help7="                        host:port,host:port,...,host:port\n"
    help8="                   Optional, is only needed for Chinese.\n"
    help9="                   If not specified, defaults to: \n"
    help10="                        localhost:9000\n"

    usage_pre ${0} "<core-nlp-dir> <back-ends>" "${help1}${help2}${help3}${help4}${help5}${help6}${help7}${help8}${help9}${help10}"
}

#Clean up after the script failed before existing
function clean() {
    #Remove the template file
    rm -f ${TEMPL_FILE}
    #Remove the Stanford Core NLP output file
    rm -f ${SNLP_OUTPUT_FILE}
}

#Allows to check the availability of java and its version
#and that the specified directory does exist.
# ${0} the name of the script
# ${1} the minimum required java version
# ${2} the specified path to the Stanford Core NLP
function check_java_and_dir() {
    #Check if the java is present
    JAVA=`which java | head -n 1`
    if [ -z "${JAVA}" ]; then
        error "Could not find Java, can not run Stanford Core NLP!"
        fail        
    fi
    #Check that the java version 
    JAVA_VERSION=$(java -version 2>&1 | awk -F '"' '/version/ {print $2}')
    if [[ "${JAVA_VERSION}" < "${1}" ]]; then
        error "Stanford Core NLP requires java version ${1}+, found ${JAVA_VERSION}!"
        fail        
    fi
    #Check that the Stanford Core NLP path does exist
    if ! [ -d "${2}" ]; then
       error "The specified Stanford Core NLP path: ${2} is not a directory!"
       fail
    fi
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

#Check if the stanford core nlp home parameter is given
export SC_NLP_DIR="."
if ! [ -z "${4}" ]; then
    SC_NLP_DIR=${4}
fi

#Check if the stanford core nlp backends list is specified
export SC_NLP_BACKENDS=""
if ! [ -z "${5}" ]; then
    SC_NLP_BACKENDS="-backends ${5}"
fi

#Check if the language is to be auto detected
export DETECTED_LANG=""
if [ "${LANGUAGE}" = "auto" ]; then
    #Execute the language detection script there is and return an error if needed
    python ${BASEDIR}/lang_detect_nltk.py ${INPUT_FILE}>${OUTPUT_FILE}
    #Check clean and fail if NOK
    check_clean_fail $?
    #If it is OK then get the detected language from the output file
    DETECTED_LANG=`cat ${OUTPUT_FILE}`
else
    DETECTED_LANG=${LANGUAGE}
fi

#Lowercase the language
DETECTED_LANG_LC=`echo ${DETECTED_LANG}| tr A-Z a-z`

#Run the pre-processing script, which one depends on
#the concrete language. Some are supported by NLTK
#some are to be done with Stanford Core NLP. Here we
#pre-build in support for chinese CBT and arabic 
case "${DETECTED_LANG_LC}" in
    chinese)
        #Check that the Stanford Core NLP is configured
        check_java_and_dir "1.8" ${SC_NLP_DIR}
        #Call the client and make it do the work
        #ToDo: When the client termination bug is fixed remove " &" at the end of the invocation command!
        java -cp "${SC_NLP_DIR}/*" -Xmx2g edu.stanford.nlp.pipeline.StanfordCoreNLPClient -props StanfordCoreNLP-${DETECTED_LANG_LC}.properties -annotators tokenize,ssplit -file ${INPUT_FILE} -outputDirectory ${WORK_DIR} -outputFormat text ${SC_NLP_BACKENDS} 1>/dev/null 2>${OUTPUT_FILE} &
        
        #########################################################################
        #NOTE: The next block is a workaround for the client bug that prevents
        #      the client from being terminated after the file is annotated
        #
            #Remember the java process id
            JAVA_PROCESS_ID=$!
            #Disown the process to prevent the killed message printed
            disown

            #Wait until the file is created
            SNLP_OUTPUT_FILE=${INPUT_FILE}.out
            while ! [ -e ${SNLP_OUTPUT_FILE} ]; do
                sleep 1
            done
            #Sleep an extra second for safety
            sleep 1
            #Kill the java process now and wait until it terminates
            kill -9 ${JAVA_PROCESS_ID}
            wait ${JAVA_PROCESS_ID}
        #ToDo: Replace with:
            #Check clean and fail if NOK
            #check_clean_fail $?
        #
        #########################################################################
        
        #Post-process the CoreNLP output into the text file
        #Create a template file for the originl text
        python ${BASEDIR}/pre_process_snlp.py -t ${TEMPL_FILE} -s ${INPUT_FILE} ${SNLP_OUTPUT_FILE} > ${OUTPUT_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?
        
        #DEBUG: Create back files for analysis
        #cp ${SNLP_OUTPUT_FILE} ${SNLP_OUTPUT_FILE}.bak
        
        #Remove the Stanford Core NLP output file
        rm -f ${SNLP_OUTPUT_FILE}
        ;;
    *)
        #Call the NLTK based pre-processing script
        python ${BASEDIR}/pre_process_nltk.py -l ${DETECTED_LANG} -t ${TEMPL_FILE} ${INPUT_FILE} > ${OUTPUT_FILE}
        #Check clean and fail if NOK
        check_clean_fail $?
esac

#DEBUG: Create back files for ananlysis
#cp ${INPUT_FILE} ${INPUT_FILE}.bak
#cp ${TEMPL_FILE} ${TEMPL_FILE}.bak
#cp ${OUTPUT_FILE} ${OUTPUT_FILE}.bak

#Output the "detected" language
echo ${DETECTED_LANG}
