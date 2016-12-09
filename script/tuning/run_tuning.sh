#!/bin/sh

###################################
#Declare and set the default values of the script parameters
CONFIG_FILE_NAME="" #No default value, is a compulsory parameter
SOURCE_TEXT_FILE="" #No default value, is a compulsory parameter
SOURCE_LANG=""
NUM_BATCHES=1 #Defaul is no concurrency and multi-threading in tuning scripts, just one batch
REFERENCE_TEXT_FILE="" #No default value, is a compulsory parameter
REFERENCE_LANGUAGE="english" #The default target language of translation
TRACE_LEVEL=1 #Default is one (1) the lowest (?) tracing level
NUM_BEST_HYPOTHESIS=1500 #In our research we use more than a 1000 hypothesis
MERT_SCRIPT_TYPE="PRO-14" #In our research we use the PRO-14 test

#Prints the program usage
# ${0} - the program name
# ${1} - the function name
function usage() {
    echo "------"
    echo "USAGE: Allows to start the tuning process for the infrastructure"
    echo "------"
    echo " ${0} --conf=<file-name> --src=<file-name> --src-language=<string> "
    echo "        --ref=<file-name> --trg-language=<string> --no-parallel=<number> "
    echo "        --trace=<number> --nbest-size=<number> --mert-script=<string>"
    echo " Where:"
    echo "    --conf=<file-name> the initial configuration file for the decoding server"
    echo "    --src=<file-name> the source text file to use with tuning"
    echo "    --src-language=<string> the language of the source text"
    echo "    --ref=<file-name> the reference translation text file to use with tuning"
    echo "    --trg-language=<string> the language of the reference text, default is ${REFERENCE_LANGUAGE}"
    echo "    --no-parallel=<number> the number of parallel threads used for tuning, default is ${NUM_BATCHES}"
    echo "    --trace=<number> the tracing level for the script, default is ${TRACE_LEVEL}"
    echo "    --nbest-size=<number> the number of best hypothesis to consider, default is ${NUM_BEST_HYPOTHESIS}"
    echo "    --mert-script=<string> the MERT script type to be used, default is '${MERT_SCRIPT_TYPE}'"
    echo ""
}

#Prints usesage, reports an error, if not empty, and fails
# ${0} - the script name
# ${1} - the error message or nothing if no message is needed
function error() {
    usage
    ERROR_MSG=${1}
    if ! [ -z "${ERROR_MSG}" ]; then
        echo "ERROR: ${ERROR_MSG}"
    fi
    exit 1
}

#If there is no arguments then print usage and exit
if [ "$#" -eq 0 ]; then
    error
fi

###################################
#Parse the script command-line parameters
for i in "$@"
do
    case $i in
        --conf=*)
            #Get the location of the server configurtion file
            CONFIG_FILE_NAME="${i#*=}"
            shift
            ;;
        --src=*)
            #Get the location of the source text file to be used for translation
            SOURCE_TEXT_FILE="${i#*=}"
            shift
            ;;
        --src-language=*)
            #Get the language in which the source text is written
            SOURCE_LANG="${i#*=}"
            shift
            ;;
        --ref=*)
            #Get the file with the reference translation for the source text
            REFERENCE_TEXT_FILE="${i#*=}"
            shift
            ;;
        --trg-language=*)
            REFERENCE_LANGUAGE="${i#*=}"
            shift
            ;;
        --no-parallel=*)
            #Get the number of parallel batches to be used when solving tuning optimization
            NUM_BATCHES="${i#*=}"
            shift
            ;;
        --nbest-size=*)
            #Get the number of the best hypothesis to consider
            NUM_BEST_HYPOTHESIS="${i#*=}"
            shift
            ;;
        --mert-script=*)
            #Get the mert script type
            MERT_SCRIPT_TYPE="${i#*=}"
            shift
            ;;
        --trace=*)
            #Get the trace level
            TRACE_LEVEL="${i#*=}"
            shift
            ;;
        *)
            #Skip the unrecognized parameter and move on
            echo "WARNING: Unrecognized script parameter: ${i}"
            shift
            ;;
    esac
done

###################################
#Check for the presene of the compulsory parameters and their correctness

#Check that the config, soure and reference files are present
if ! [ -e ${CONFIG_FILE_NAME} ]; then
    error "The configuration file ${CONFIG_FILE_NAME} is not found!"
fi
if ! [ -e ${SOURCE_TEXT_FILE} ]; then
    error "The source text file ${SOURCE_TEXT_FILE} is not found!"
fi
if ! [ -e ${REFERENCE_TEXT_FILE} ]; then
    error "The reference text file ${REFERENCE_TEXT_FILE} is not found!"
fi

#Check that the source language is defined
if [ -z "${SOURCE_LANG}" ]; then
    error "The source language is not defined!"
fi

###################################
#Call the tuner script with the proper parameter values

#Just in case re-move the language environment value
unset LANG

#Get the directory where this script is located
BASEDIR=$(dirname "$0")

#Define the folder where the Mega-m is located
MEGA_M_HOME_DIR=$BASEDIR/megam_0.92/

DATE_TIME=`date`
echo "Starting tuning on: ${HOSTNAME} at: ${DATE_TIME}"

$BASEDIR/scripts/tuner.pl --src=${SOURCE_TEXT_FILE} --node-scoring --ref=${REFERENCE_TEXT_FILE} --decoder=$BASEDIR/start_infra.sh --external-path=${MEGA_M_HOME_DIR} --conf=${CONFIG_FILE_NAME} --no-parallel=${NUM_BATCHES} --trace=${TRACE_LEVEL} --nbest-size=${NUM_BEST_HYPOTHESIS} --src-language=${SOURCE_LANG} --mert-script=${MERT_SCRIPT_TYPE} --trg-language=${REFERENCE_LANGUAGE} --experiment-dir="."