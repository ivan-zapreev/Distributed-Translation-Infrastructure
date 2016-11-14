#Prints the program usage
# ${0} - the program name
# ${1} - the function name
# ${2} - the list of additional parameters (optional)
# ${3} - the additional parameter's description (optional)
function usage_post(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " ${0} <work-dir> <job-uid> <language> ${2}"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the post-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.post.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.post.out.txt"
  echo "    <language> - the post-processor input text language."
  if ! [ -z "${3}" ]; then
    echo "${3}"
  fi
}

#Prints the program usage
# ${0} - the program name
# ${1} - the function name
# ${2} - the list of additional parameters (optional)
# ${3} - the additional parameter's description (optional)
function usage_pre(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " ${0} <work-dir> <job-uid> <language> ${2}"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the pre-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.pre.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.pre.out.txt"
  echo "    <language> - the pre-processor input text language."
  echo "                 If set to \"auto\", the language is to be detected."
  if ! [ -z "${3}" ]; then
    echo "${3}"
  fi
}

#Reports an error, does not exit
# ${0} - the script name
# ${1} - the error message
function error() {
   echo "ERROR in ${0}: ${1}"
}

#Allows the program to fail
function fail() {
   exit 1
}
