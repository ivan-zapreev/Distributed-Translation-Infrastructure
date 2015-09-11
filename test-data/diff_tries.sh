#!/bin/sh

NUM_PARAMS=3
if [ "$#" -ne ${NUM_PARAMS} ]; then
   echo "Illegal number of parameters: $# expected ${NUM_PARAMS}"
   echo "1 - the platform: {Linux, MaxOS, Centos}"
   echo "2 - the model to run on"
   echo "3 - the query input file to use"
   exit 1
fi

FILTER=' | grep -v "Gram ctx hash" | grep -v "CPU" | grep -v "vmsize" | grep -v "Cultivating" | grep -v "Counting words in ARPA file" | grep -v " memory allocation strategy."'
echo ${FILTER}

echo "c2wa"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} c2wa info3 ${FILTER} > release.c2wa.out"
echo "c2dm"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} c2dm info3 ${FILTER} > release.c2dm.out"
echo "w2ca"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} w2ca info3 ${FILTER} > release.w2ca.out"
echo "w2ch"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} w2ch info3 ${FILTER} > release.w2ch.out"
echo "c2dh"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} c2dh info3 ${FILTER} > release.c2dh.out"
echo "g2dm"
eval "../dist/Release__${1}_/back-off-language-model-smt ${2} ${3} g2dm info3 ${FILTER} > release.g2dm.out"

echo "----> c2wa vs. c2dm"
diff release.c2wa.out release.c2dm.out > diff.c2wa.c2dm.out
cat diff.c2wa.c2dm.out | wc -l

echo "----> c2wa vs. w2ca"
diff release.c2wa.out release.w2ca.out > diff.c2wa.w2ca.out
cat diff.c2wa.w2ca.out | wc -l

echo "----> c2wa vs. w2ch"
diff release.c2wa.out release.w2ch.out > diff.c2wa.w2ch.out
cat diff.c2wa.w2ch.out | wc -l

echo "----> c2wa vs. c2dh"
diff release.c2wa.out release.c2dh.out > diff.c2wa.c2dh.out
cat diff.c2wa.c2dh.out | wc -l

echo "----> c2wa vs. g2dm"
diff release.c2wa.out release.g2dm.out > diff.c2wa.g2dm.out
cat diff.c2wa.g2dm.out | wc -l

echo "------------------------------"

echo "----> c2dm vs. c2wa"
diff release.c2dm.out release.c2wa.out > diff.c2dm.c2wa.out
cat diff.c2dm.c2wa.out | wc -l

echo "----> c2dm vs. w2ca"
diff release.c2dm.out release.w2ca.out > diff.c2dm.w2ca.out
cat diff.c2dm.w2ca.out | wc -l

echo "----> c2dm vs. w2ch"
diff release.c2dm.out release.w2ch.out > diff.c2dm.w2ch.out
cat diff.c2dm.w2ch.out | wc -l

echo "----> c2dm vs. c2dh"
diff release.c2dm.out release.c2dh.out > diff.c2dm.c2dh.out
cat diff.c2dm.c2dh.out | wc -l

echo "----> c2wa vs. g2dm"
diff release.c2dm.out release.g2dm.out > diff.c2dm.g2dm.out
cat diff.c2dm.g2dm.out | wc -l
