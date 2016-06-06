#!/bin/sh

if [ "$#" -eq 0 ]; then
  echo "Usage: $0 <lattice-dir> <result-file-name> <sent-lattice-ext> <set-scores-ext>"
  echo "    <lattice-dir> - the diurectory with the lattice files"
  echo "    <result-file-name> - the file name to be used for the combined lattice data"
  echo "    <sent-lattice-ext> - the lattice file extension for a sentence, default is 'lattice'"
  echo "    <set-scores-ext> - the feature scores file extension for a sentence, default is 'feature_scores'"
  exit 1
fi

#Store the directory in which the lattice files reside
export LATTICES_DERECTORY=${1}
if ! [ -e "${LATTICES_DERECTORY}" ]; then
  echo "ERROR: ${LATTICES_DERECTORY} not found"
  exit 1
fi
if ! [ -d "${LATTICES_DERECTORY}" ]; then
  echo "ERROR: ${LATTICES_DERECTORY} not a directory"
  exit 1
fi

#Store the name of the output file
export OUTPUT_FILE_NAME=${2}
if [ -z "${OUTPUT_FILE_NAME}" ]; then
  echo "ERROR: <result-file-name> is not defined"
  exit 1
fi

#Store the lattice file extention
export LATTICE_FILE_EXT=${3}
if [ -z "${LATTICE_FILE_EXT}" ]; then
    export LATTICE_FILE_EXT="lattice"
fi

#Store the feature scores file extention
export SCORE_FILE_EXT=${4}
if [ -z "${SCORE_FILE_EXT}" ]; then
    export SCORE_FILE_EXT="feature_scores"
fi

echo "Searhcing for the lattices in the folder:" ${LATTICES_DERECTORY}
echo "------------------------------------"

#Move to the lattices folder
export INITIAL_DIRECTORY=`pwd`
cd ${LATTICES_DERECTORY}

#Collect the lattice file names
export SENTENCE_FILES=`ls *.${LATTICE_FILE_EXT} | cut -f 1 -d '.' | sort -k 1n`

#Define the lattices and scores output file names
export LATTICES_OUT_FILE="${OUTPUT_FILE_NAME}.lattices"
export SCORES_OUT_FILE="${OUTPUT_FILE_NAME}.feature_scores"

#Re-set the files
rm -f ${LATTICES_OUT_FILE} ${SCORES_OUT_FILE}

#Iterate through the files and combine them into lattices and scores file
for FILE_NAME in ${SENTENCE_FILES}; do
  echo "Considering ${FILE_NAME}.${LATTICE_FILE_EXT} and ${FILE_NAME}.${SCORE_FILE_EXT}"
  
  #Compute the sentence id
  export SENT_ID=$(expr ${FILE_NAME} - 1)

  #Append the lattice data
  echo "<SENT ID=${SENT_ID}>" >> ${LATTICES_OUT_FILE}
  cat ${FILE_NAME}.${LATTICE_FILE_EXT} >> ${LATTICES_OUT_FILE}
  echo "</SENT>" >> ${LATTICES_OUT_FILE}

  #Append the scores data
  echo "<SENT ID=${SENT_ID}>" >> ${SCORES_OUT_FILE}
  cat ${FILE_NAME}.${SCORE_FILE_EXT} >> ${SCORES_OUT_FILE}
  echo "</SENT>" >> ${SCORES_OUT_FILE}
done

#Archive the resulting files
gzip ${LATTICES_OUT_FILE}
gzip ${SCORES_OUT_FILE}

#Report on the results
echo "------------------------------------"
echo "The resulting files are: ${LATTICES_OUT_FILE}.gz and ${SCORES_OUT_FILE}.gz"

#Move back to the original folder
cd ${INITIAL_DIRECTORY}
