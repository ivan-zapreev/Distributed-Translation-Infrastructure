#!/bin/bash

#Prints the program usage
# ${0} - the program name
function usage(){
  echo "------"
  echo "USAGE:"
  echo "------"
  echo " $0 <num-processes> <config-file> <source-file> <source-lang> <target-lang>"
  echo "    <num-proc> - the number of bpbd-clients to run in parallel"
  echo "    <config-file> - the client configuration file"
  echo "    <source-file> - the source text file"
  echo "    <source-lang> - the source language or auto"
  echo "    <target-lang> - the target language"
}

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a stress testing script for the translation system."
  echo "   It's purpose is to run multiple translation clients in parallel"
  usage ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   The main purpose of the script is to stress load the infrastructure "
  echo "   and see if any errors or deadlocks or alike are occurring."
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

#Check if the configuration file is defined
if [ -z "${2}" ]; then
   error "<config-file> is not defined"
   fail
fi

#Check if the source text is defined
if [ -z "${3}" ]; then
   error "<source-file> is not defined"
   fail
fi

#Check if the source language is defined
if [ -z "${4}" ]; then
   error "<source-lang> is not defined"
   fail
fi

#Check if the target language is defined
if [ -z "${5}" ]; then
   error "<target-lang> is not defined"
   fail
fi

export FILTER='s/.res.*.txt/.res.N.txt/g'

#Clearing the previous logs
echo "Clearing the previous logs ..."
rm -f ./proc.*.log ./output.res.*.txt ./output.res.*.txt.log

BASEDIR=$(dirname "$0")/../../build/

#Run the control
echo "Performing a control run ..."
${BASEDIR}/bpbd-client -c ${2} -I ${BASEDIR}/${3} -i ${4} -O ./output.res.0.txt -o ${5} | sed -e ${FILTER}  > ./proc.0.log

#Run the process instances
echo "Starting ${1} parallel clients ..."
for i in `seq 1 ${1}`; do
  #echo "Starting process: ${i}"
  ${BASEDIR}/bpbd-client -c ${2} -I ${BASEDIR}/${3} -i ${4} -O ./output.res.${i}.txt -o ${5} | sed -e ${FILTER} > ./proc.${i}.log &
done

echo "Waiting for the client processes to finish..."
wait

#Run the diffs to check that thre results are overall the same
echo "Analysing the differences..."
for i in `seq 1 ${1}`; do
  export DIFF_RESULT=`diff ./proc.0.log ./proc.${i}.log`
  if [ ! -z "${DIFF_RESULT}" -a "${DIFF_RESULT}" != "" ]; then
     echo "------------------------------------------------"
     echo "| Diff run logs: 0 vs. ${i}:"
     diff ./proc.0.log ./proc.${i}.log
  fi
  export DIFF_RESULT=`diff ./output.res.0.txt ./output.res.${i}.txt`
  if [ ! -z "${DIFF_RESULT}" -a "${DIFF_RESULT}" != "" ]; then
     echo "------------------------------------------------"
     echo "| Diff target texts: 0 vs. ${i}:"
     diff ./output.res.0.txt ./output.res.${i}.txt
  fi
  export DIFF_RESULT=`diff ./output.res.0.txt.log ./output.res.${i}.txt.log`
  if [ ! -z "${DIFF_RESULT}" -a "${DIFF_RESULT}" != "" ]; then
     echo "------------------------------------------------"
     echo "| Diff text logs: 0 vs. ${i}:"
     diff ./output.res.0.txt.log ./output.res.${i}.txt.log
  fi
done

echo "Done!"
