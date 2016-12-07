#!/bin/sh

unset LANG

if [ $OISTERHOME ]; then
    echo "\$OISTERHOME=$OISTERHOME"
else 
    echo environment variable OISTERHOME must be set:
    echo export OISTERHOME=/path/to/oister/distribution
    exit 1
fi

echo "Usage: ${0} <result-file> <source-file> <reference-file>"
echo "    <result-file> - the TEXT translation file name without .res.txt at the end"
echo "    <source-file> - the XML source file name without .src.xml"
echo "    <reference-file> - the XML reference translation file name  without .ref.xml"

#The actual translation result file name without .res.txt in the end
RESULT_PLAIN=${1}
#The source german text xml without .src.xml
SOURCE_XML=${2}
#The reference translation xml without .ref.xml
TARGET_XML=${3}

RES_TXT=${RESULT_PLAIN}.res.txt
export RES_XML=${RESULT_PLAIN}.res.xml
export SRC_XML=${SOURCE_XML}.src.xml
export REF_XML=${TARGET_XML}.ref.xml

$OISTERHOME/evaluation/mteval-plain2xml.pl --tst-plain=${RES_TXT} --tst-xml=${RES_XML}  --src-xml=${SRC_XML} --src-lang=German --trg-lang=English --untokenize=1 --rm-non-ascii=1 --truecase=0

$OISTERHOME/evaluation/bleu-xml.pl --s=${SRC_XML} --r=${REF_XML} --t=${RES_XML} --ter --genre-breakdown
