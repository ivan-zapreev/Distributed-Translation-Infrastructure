#Prints the program usage
# ${0} - the program name
function usage_post(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <work-dir> <job-uid> <language>"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the post-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.post.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.post.out.txt"
  echo "    <language> - the post-processor input text language."
}

#Prints the program usage
# ${0} - the program name
function usage_pre(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <work-dir> <job-uid> <language>"
  echo "    <work-dir> - the work directory to read/write files from/into"
  echo "    <job-uid> - the unique identifier of the pre-processor job."
  echo "               The job input file should have name:"
  echo "                   <job-uid>.pre.in.txt"
  echo "               The job output file will get name:"
  echo "                   <job-uid>.pre.out.txt"
  echo "    <language> - the pre-processor input text language."
  echo "                 If set to \"auto\", the language is to be detected."
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
