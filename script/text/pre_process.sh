#!/bin/sh
#
# File:   pre_process.sh
# Author: Dr. Ivan S. Zapreev
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.#
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Created on November 14, 2016, 11:07 AM
#

#Prints the program info
# ${0} - the program name
function info() {
  echo "------"
  echo "SHORT:"
  echo "------"
  echo "   This is a dummy script for pre-processing of a text in a given language."
  usage_pre ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   Perform pre-processing of the given utf-8 text. Is to be called from"
  echo "   the bpbd-processor server application. The script must work as follows:"
  echo "      1. if <language> == \"auto\":"
  echo "        1.1. True: if language detection is enabled"
  echo "           1.1.1. True: detect a language:"
  echo "              1.1.1.1. Detected: remember the source language"
  echo "              1.1.1.2. Failed: report an error to the standard output and finish"
  echo "           1.1.2. False: report an error to the standard output and finish"
  echo "        1.2. False: nothing"
  echo "      2. Check if the language pre-processor is available"
  echo "        2.1. Available: call the language pre-processor"
  echo "           2.1. Success: output the language string to the standard output"
  echo "           2.1. Fail: report an error to the standard output and finish"
  echo "        2.1. Not available: check if the default pre-processor is available"
  echo "           2.1.1. Available: call the default pre-processor"
  echo "              2.1.1.1. Success: output the language string to the standard output"
  echo "              2.1.1.2. Fail: report an error to the standard output and finish"
  echo "           2.1.2. Not available: report an error to the standard output and finish"
  echo "   Pre-processing might include, but is not limited by: "
  echo "      * language detection - detecting the language in which the text is written"
  echo "      * tokenization - splitting the language words and punctuation marks with spaces"
  echo "      * lowercasing  - turning the text into the lowercase"
  echo "      * unification  - unifying the text by substituting longer utf-8 symbols with shorter ones"
  echo "   In addition to mentioned above, the result of this script must be two fold:"
  echo "      * If the pre-processing went without errors:"
  echo "         - The exit code must be 0"
  echo "         - The only data written to standard output must be the <language>"
  echo "         - The pre-processed text must be written into the file:"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "      * If the pre-processing failed:"
  echo "         - The exit code must be 1"
  echo "         - The only data written to standard output must be the error message, max 1023 characters long"
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   * The bpbd-processor application after consuming the file:"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "     will attempt to delete both pre-processing job files:"
  echo "              <work-dir>/<job-uid>.pre.in.txt"
  echo "              <work-dir>/<job-uid>.pre.out.txt"
  echo "     So, if these files are to be kept for, e.g. post-processing,"
  echo "     it is the responsibility of the script to do so."
  echo "   * This is a dummy script that only emulates the real behavior, therefore:"
  echo "      * The script only detects the input language to be \"German\""
  echo "      * The script only copies the input file into the output"
}

#Get this script actual location to find utility scripts
SCRIPT=$(readlink "${0}")
BASEDIR=$(dirname "${SCRIPT}}")

#Include the utils
. ${BASEDIR}/process_utils.sh

#Define the script type
export SCRIPT_TYPE="pre"

#Process the script parameters
. ${BASEDIR}/process_params.sh

#Check if the language is to be auto detected
if [ "${LANGUAGE}" = "auto" ]; then
   LANGUAGE="German";
fi

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

#Output the "detected" language
echo ${LANGUAGE}
