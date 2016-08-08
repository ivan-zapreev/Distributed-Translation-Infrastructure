#!/bin/sh

#Prints the program usage
# ${0} - the program name
function usage(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <num-processes> <trans-uri> <proc-uri>"
  echo "    <num-proc> - the number of processes to run in parallel"
  echo "    <trans-uri> - the URL of the translation server, with the port"
  echo "    <proc-uri> - the URL of the text processor server, with the port"
  echo "    <source-file> - the source text file"
  echo "    <source-lang> - the source language or auto"
  echo "    <target-lang> - the target language or auto"
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a stress loading script forfor the translation sytem."
  echo "   It's purpose is to run multiple translation clients in parallel"
  usage ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   The main purpose of the script is to stress load the infrastructure "
  echo "   and see if there is any errors or deadlocks or alike are occuring."
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

#If the script is called with no arguments then show the info message
if [ "$#" -eq 0 ]; then
   error "Improper number of arguments!"
   info ${0}
   fail
fi

#Check if the number of processes is defined
if [ -z "${1}" ]; then
   error "<num-proc> is not defined"
   fail
fi

#Check if the translation server uri is defined
if [ -z "${2}" ]; then
   error "<trans-uri> is not defined"
   fail
fi

#Check if the text processor server uri is defined
if [ -z "${3}" ]; then
   error "<proc-uri> is not defined"
   fail
fi

#Check if the source text is defined
if [ -z "${4}" ]; then
   error "<source-file> is not defined"
   fail
fi

#Check if the source language is defined
if [ -z "${5}" ]; then
   error "<source-lang> is not defined"
   fail
fi

#Check if the target language is defined
if [ -z "${6}" ]; then
   error "<target-lang> is not defined"
   fail
fi

#Run the process instances
for i in `seq 1 ${1}`; do
  echo "Starting process: ${i}"
  bpbd-client -d info3 -I ${4} -i ${5} -O ./output.res.${i}.txt -o ${6} -s ${2} -c -r ${3} -p ${3} > proc.${i}.log &
done
echo "Waiting for the processes to finish..."
wait
