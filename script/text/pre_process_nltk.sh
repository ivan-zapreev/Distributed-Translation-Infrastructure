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

#Clean up after the script before existing
function clean() {
    #Remove the template file
    rm -f ${TEMPL_FILE}
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

#Lowercase the language
DETECTED_LANG=`echo ${LANGUAGE}| tr A-Z a-z`

#Run the pre-processing script, which one depends on
#the concrete language. Some are supported by NLTK
#some are to be done with Stanford Core NLP. Here we
#pre-build in support for chinese CBT and arabic 
case "${DETECTED_LANG}" in
    chinese)
        #Check that the Stanford Core NLP is configured
        check_java_and_dir "1.8" ${SC_NLP_DIR}
        #Call the client and make it do the work
        java -cp "${SC_NLP_DIR}/*" -Xmx2g edu.stanford.nlp.pipeline.StanfordCoreNLPClient -props StanfordCoreNLP-${DETECTED_LANG}.properties -annotators tokenize,ssplit -file ${INPUT_FILE} -outputDirectory ${WORK_DIR} -outputFormat text ${SC_NLP_BACKENDS} 2>&1 1>${OUTPUT_FILE}
        #Check on the returned code
        rc=$?
        if [[ $rc != 0 ]]; then
            echo `cat ${OUTPUT_FILE}`
            clean
            exit $rc;
        fi
        #ToDo: Post-process the CoreNLP output into the text file
        #For now just rename the output file, give the right name
        mv -f ${INPUT_FILE}.out ${OUTPUT_FILE}
        ;;
    *)
        #Call the NLTK based pre-processing script
        python ${BASEDIR}/pre_process_nltk.py -l ${DETECTED_LANG} -t ${TEMPL_FILE} ${INPUT_FILE} > ${OUTPUT_FILE}
        #Check on the returned code from the script
        rc=$?
        if [[ $rc != 0 ]]; then
            echo `cat ${OUTPUT_FILE}`
            clean
            exit $rc;
        fi
esac

#DEBUG: Create back files for ananlysis
#cp ${INPUT_FILE} ${INPUT_FILE}.bak
#cp ${TEMPL_FILE} ${TEMPL_FILE}.bak
#cp ${OUTPUT_FILE} ${OUTPUT_FILE}.bak

#Output the "detected" language
echo ${DETECTED_LANG}
