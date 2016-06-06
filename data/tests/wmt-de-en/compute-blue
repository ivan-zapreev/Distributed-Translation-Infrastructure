#!/bin/sh

unset LANG

if [ $OISTERHOME ]; then
    echo "\$OISTERHOME=$OISTERHOME"
else 
    echo environment variable OISTERHOME must be set:
    echo export OISTERHOME=/path/to/oister/distribution
    exit 1
fi

echo "Usage: "
echo "    ${1} - the actual translation	result file name without .res.txt at the end"
echo "    ${2} - the source german text	xml without .src.xml"
echo "    ${3} - the reference translation xml without .ref.xml"

#The actual translation result file name without .res.txt in the end
export RESULT_PLAIN=${1} 
#The source german text xml without .src.xml
export SOURCE_XML=${2}
#The reference translation xml without .ref.xml
export TARGET_XML=${3}

export RES_TXT=${RESULT_PLAIN}.res.txt
export RES_0_TXT=.0.${RES_TXT}
export RES_1_TXT=.1.${RES_TXT}
export RES_2_TXT=.2.${RES_TXT}
export RES_XML=${RESULT_PLAIN}.res.xml
export SRC_XML=${SOURCE_XML}.src.xml
export REF_XML=${TARGET_XML}.ref.xml

sed -e 's/<error>: //' ${RES_TXT} > ${RES_0_TXT}
sed -e '/^<info>/d' ${RES_0_TXT} > ${RES_1_TXT}
sed -e 's/<finished>: //' ${RES_1_TXT} > ${RES_2_TXT}

$OISTERHOME/evaluation/mteval-plain2xml.pl --tst-plain=${RES_2_TXT} --tst-xml=${RES_XML}  --src-xml=${SRC_XML} --src-lang=German --trg-lang=English --untokenize=1 --rm-non-ascii=1 --truecase=0

rm -f ${RES_0_TXT} ${RES_1_TXT} ${RES_2_TXT}

#$OISTERHOME/resources/software/mteval/mteval-v13a/current/mteval-v13a_cm.pl -s ${SRC_XML} -r ${REF_XML} -t ${RES_XML} -b 1> $OUTSTEM.bleu-nocased

$OISTERHOME/evaluation/bleu-xml.pl --s=${SRC_XML} --r=${REF_XML} --t=${RES_XML} --ter --genre-breakdown
