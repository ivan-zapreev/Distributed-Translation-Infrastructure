#!/bin/bash

BASEDIR=$(dirname "$0")
BUILDPATH="$BASEDIR/../../build"
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

if [ "$createFeatureMapping" = true ] 
then
    $BUILDPATH/bpbd-server -c $configFile -f >& /dev/null
else
    pids=$(pidof bpbd-server)
    pidsArray=($pids)
    if [ -z "$pids" ]
    then
        $BUILDPATH/bpbd-server -c $configFile -d error > /dev/null &
    else
        for i in "${pidsArray[@]}"
        do
            #echo pid:$i
            kill -9 $i
        done
        $BUILDPATH/bpbd-server -c $configFile -d error > /dev/null &
    fi

    clientMessage=''
    res=1
    counter=0
    while [ -z "$clientMessage" -o "$res" -eq 1 ]; do
        sleep 10s
        clientMessage=$($BUILDPATH/bpbd-client -I $srcFile -i $srcLang -O $trgFile -o $trgLang)
        res=$?
        cat "$clientMessage" >& client.log
        let counter=counter+1
        echo attempt number $counter >> client.log
    done
    $BASEDIR/combine-lattices.sh $expDir/lattices ../$trgFile lattices feature_scores $no_parallel >& combine.log
fi


