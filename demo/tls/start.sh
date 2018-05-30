#!/bin/bash

export PROJECT_PATH=../..
export BINARY_PATH=${PROJECT_PATH}/build

function extract_if_not_present() {
    if [ -f "${1}" ]; then
        echo "INFO: ${1} is already extracted!"
    else
        echo "INFO: Exracting ${1} ... "

        #Define the requested archive name
        REC_ARCH_NAME=${1}.zip

        #Check if we have a multi-part archive, then
        #we need to first restore the complete file
        if [ ! -f "${REC_ARCH_NAME}" ]; then
            ACT_ARCH_NAME=.${REC_ARCH_NAME}
            #Combine the archive parts
            cat ${REC_ARCH_NAME}.* > ${ACT_ARCH_NAME}
        else
            ACT_ARCH_NAME=${REC_ARCH_NAME}
        fi

        #Unzip the actual archive
        unzip ${ACT_ARCH_NAME}

        #Remove the actual archive if this is a multi-part one
        if [ ! -f "${REC_ARCH_NAME}" ]; then
            rm -f ${ACT_ARCH_NAME}
        fi

        echo "Done"
    fi
}

echo "INFO: Using the project path: ${PROJECT_PATH}"

#Link the processor scripts
ln -f -s ${PROJECT_PATH}/script/text/pre_process.sh .pre_process.sh
ln -f -s ${PROJECT_PATH}/script/text/post_process.sh .post_process.sh

#Extracting the models
echo "INFO: Starting Extracting the model files:"
cd ./models
extract_if_not_present chinese.english.head.10.rm
extract_if_not_present chinese.english.head.10.tm
extract_if_not_present de-en-1-10.000.rm
extract_if_not_present de-en-1-10.000.tm
extract_if_not_present english.bitext.lm
extract_if_not_present e_00_1000.lm
cd ..

#First start the servers
echo "INFO: Starting Translation Servers:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server-tls.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server-no-tls.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The servers are started"

#Now start the balancer
echo "INFO: Starting Load Balancing Server:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-balancer -d info3 -c ./configs/balancer-mixed-tls.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The load balancer is started"

#Start the pre/post processors
echo "INFO: Starting Pre/Post-Processing Servers:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-processor -d info3 -c ./configs/processor-tls.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
cmd="screen -d -m ${BINARY_PATH}/bpbd-processor -d info3 -c ./configs/processor-no-tls.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The pre/post-processors are started"
echo ""
echo "WARN: Use 'screen -r' to connect to the one of the 5 running servers"
echo "WARN: Missing screens means failed-to-run servers!"
echo ""
echo "INFO: Please press enter to STOP the servers and finish..."
read input_variable

#Kill the processes
pkill -9 bpbd-server
pkill -9 bpbd-balancer bpbd-processor
pkill -9 bpbd-processor

#Remove the linked scripts
rm -rf .proc_text .pre_process.sh .post_process.sh english.txt english.txt.log ./models/*.lm ./models/*.tm ./models/*.rm

