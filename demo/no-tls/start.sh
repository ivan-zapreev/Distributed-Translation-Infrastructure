#!/bin/sh

export PROJECT_PATH=../../../../Distributed-Translation-Infrastructure
export BINARY_PATH=${PROJECT_PATH}/build

echo "INFO: Using the project path: ${PROJECT_PATH}"

#Echo link the processor scripts
ln -f -s ${PROJECT_PATH}/script/text/pre_process.sh .
ln -f -s ${PROJECT_PATH}/script/text/post_process.sh .

#First start the servers
echo "INFO: Starting Translation Servers:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server.1.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server.2.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server.3.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
cmd="screen -d -m ${BINARY_PATH}/bpbd-server -d info3 -c ./configs/server.4.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The servers are started"

#Now start the balancer
echo "INFO: Starting Load Balancing Server:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-balancer -d info3 -c ./configs/balancer.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The load balancer is started"

#Start the pre-post processor
echo "INFO: Starting Pre/Post-Processing Server:"
cmd="screen -d -m ${BINARY_PATH}/bpbd-processor -d info3 -c ./configs/processor.cfg"
echo "INFO1:    ${cmd}" && eval ${cmd}
echo "INFO: The pre/post-processor is started"
echo ""
echo "WARN: Use 'screen -r' to connect to the one of the 6 running servers"
echo "WARN: Missing screens means failed-to-run servers!"
echo ""
echo "INFO: Please press enter to STOP the servers and finish..."
read input_variable

#Kill the processes
pkill -9 bpbd-server
pkill -9 bpbd-balancer bpbd-processor
pkill -9 bpbd-processor
