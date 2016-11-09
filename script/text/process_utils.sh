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
