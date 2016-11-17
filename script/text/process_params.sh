#If the script is called with no arguments then show the info message
if [ "$#" -eq 0 ]; then
   info ${0}
   error "Improper number of arguments!"
   fail
fi

#Check if the work directory param is defined and the directory is present
if [ -z "${1}" ]; then
   error "<work-dir> is not defined"
   fail
fi
if ! [ -e "${1}" ]; then
   error "${1} not found!"
   fail
fi
if ! [ -d "${1}" ]; then
   error "${1} is not a directory!"
   fail
fi

#Check if the job uid param is defined
if [ -z "${2}" ]; then
   error "<job-uid> is not defined"
   fail
fi

#Check if the language param is defined
if [ -z "${3}" ]; then
   error "<language> is not defined"
   fail
fi

#Store the directory in which the processor files reside
export WORK_DIR=${1}

#Read the job id and check on it, create input output file names
#and the template file name for the text structure preservation
export JOB_UID=${2}
export INPUT_FILE=${WORK_DIR}/${JOB_UID}.${SCRIPT_TYPE}.in.txt
export TEMPL_FILE=${WORK_DIR}/${JOB_UID}.templ
export OUTPUT_FILE=${WORK_DIR}/${JOB_UID}.${SCRIPT_TYPE}.out.txt

if ! [ -e "${INPUT_FILE}" ]; then
   error "${INPUT_FILE} could not be found!"
   fail
fi

#Get the language
export LANGUAGE=${3}

#Check if the language can be auto detected
if [ "${SCRIPT_TYPE}" = "post" ]; then
   if [ "${LANGUAGE}" = "auto" ]; then
      error "The language auto detection is not allowed!"
      fail
   fi
fi