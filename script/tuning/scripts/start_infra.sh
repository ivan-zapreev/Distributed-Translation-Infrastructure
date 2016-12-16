#!/bin/bash

#Define the client and server location directory, relative to this script location
BASEDIR=$(dirname "$0")
BUILDPATH="$BASEDIR/../../../build"
SERVER_NAME=bpbd-server
CLIENT_NAME=bpbd-client

#Parse the sript parameters
for i in "$@"
do
    case $i in
        --conf=*)
            configFile="${i#*=}"
            shift
            ;;
        --src=*)
            srcFile="${i#*=}"
            shift
            ;;
        --src-language=*)
            srcLang="${i#*=}"
            shift
            ;;
        --no-parallel=*)
            no_parallel="${i#*=}"
            shift
            ;;
        --trg-language=*)
            trgLang="${i#*=}"
            shift
            ;;
        --trg=*)
            trgFile="${i#*=}"
            shift
            ;;
        --experiment-dir=*)
            expDir="${i#*=}"
            shift
            ;;
        --create-features-mapping)
            createFeatureMapping=true
            shift
            ;;
        --default)
            defaultArg=YES
            shift
            ;;
        *)

            ;;
    esac
done

rm -f $expDir/lattices/*

if [ "$createFeatureMapping" = true ]; then
    #Generate the feature to id mapping
    $BUILDPATH/${SERVER_NAME} -c $configFile -f >& /dev/null
else
    #Check if the servers are running if yes then shut them all down
    pids=$(pidof ${SERVER_NAME})
    pidsArray=($pids)
    if ! [ -z "$pids" ]; then
        for i in "${pidsArray[@]}"
        do
            kill -9 $i
        done
    fi
    
    #Start a new server instance
    $BUILDPATH/${SERVER_NAME} -c $configFile -d error > /dev/null &
    SERVER_PID=$!
    
    #Log the server pid into stderr for possible future killing
    >&2 echo "${SERVER_NAME} pid=${SERVER_PID}"

    #Wait until the server is fully loaded and then send the translation request
    clientMessage=''
    res=1
    counter=0
    while [ -z "$clientMessage" -o "$res" -eq 1 ]; do
        sleep 10s
        clientMessage=$($BUILDPATH/${CLIENT_NAME} -I $srcFile -i $srcLang -O $trgFile -o $trgLang)
        res=$?
        cat "$clientMessage" >& client.log
        let counter=counter+1
        echo attempt number $counter >> client.log
    done
    
    #Once the translation is ready, combine the generated lattice files
    $BASEDIR/combine_lattices.sh $expDir/lattices ../$trgFile lattices feature_scores $no_parallel >& combine.log
fi


