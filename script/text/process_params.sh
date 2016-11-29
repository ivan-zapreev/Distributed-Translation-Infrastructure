#If the script is called with no arguments then show the info message
if [ "$#" -eq 0 ]; then
   info ${0}
   error "Improper number of arguments!"
   fail
fi

#Initialize the variables
WORK_DIR=""
JOB_UID=""
LANGUAGE=""

#Save the parameters, so that they can be parsed again later
SCRIPT_PARAMS=("$@")

#Process the standard input parameters one by one
for i in "$@"
do
    case $i in
        --work-dir=*)
            #Store the directory in which the processor files reside
            WORK_DIR="${i#*=}"
            shift
            ;;
        --job-uid=*)
            #Get the job uid parameter
            JOB_UID="${i#*=}"
            shift
            ;;
        --lang=*)
            #Get the language
            LANGUAGE="${i#*=}"
            shift
            ;;
        *)
            #Do nothing, this is some other parameter to be parsed elsewhere
            shift
            ;;
    esac
done

#############################################################
#Check if the work directory param is well defined
if [ -z "${WORK_DIR}" ]; then
    info
    error "The <work-dir> parameter is not defined!"
    fail
fi
if ! [ -e "${WORK_DIR}" ]; then
    info
    error "The work directory ${WORK_DIR} is not found!"
    fail
fi
if ! [ -d "${WORK_DIR}" ]; then
    info
    error "${WORK_DIR} is not a directory!"
    fail
fi

#############################################################
#Check if the job uid param is well defined
if [ -z "${JOB_UID}" ]; then
    info
    error "The <job-uid> parameter is not defined!"
    fail
fi
#Define the input, output and template file
#names for the text structure preservation
INPUT_FILE=${WORK_DIR}/${JOB_UID}.${SCRIPT_TYPE}.in.txt
TEMPL_FILE=${WORK_DIR}/${JOB_UID}.templ
OUTPUT_FILE=${WORK_DIR}/${JOB_UID}.${SCRIPT_TYPE}.out.txt
#Check that the input file does exist
if ! [ -e "${INPUT_FILE}" ]; then
    info
    error "The input file ${INPUT_FILE} could not be found!"
    fail
fi

#############################################################
#Check if the language param is well defined
if [ -z "${LANGUAGE}" ]; then
    info
    error "The <language> parameter is not defined!"
    fail
fi
#Check if the language can be auto detected
if [ "${SCRIPT_TYPE}" = "post" ]; then
    if [ "${LANGUAGE}" = "auto" ]; then
        info
        error "The language auto detection is not allowed!"
        fail
    fi
fi

#Restore the parameters, as they might be parsed again later
set -- ${SCRIPT_PARAMS[@]}
