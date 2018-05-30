#!/bin/bash
#
# File:   post_process.sh
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
  echo "   This is a dummy script for post-processing of a text in a given language."
  usage_post ${0}
  echo "------"
  echo "PURPOSE:"
  echo "------"
  echo "   Perform post-processing of the given utf-8 text. Is to be called from"
  echo "   the bpbd-processor server application. The script must work as follows:"
  echo "      1. if <language> == \"auto\":"
  echo "        1.1. True: report an error to the standard output and finish"
  echo "        1.2. False: nothing"
  echo "      2. Check if the language post-processor is available"
  echo "        2.1. Available: call the language post-processor"
  echo "           2.1. Success: output the language string to the standard output"
  echo "           2.1. Fail: report an error to the standard output and finish"
  echo "        2.1. Not available: check if the default post-processor is available"
  echo "           2.1.1. Available: call the default post-processor"
  echo "              2.1.1.1. Success: output the language string to the standard output"
  echo "              2.1.1.2. Fail: report an error to the standard output and finish"
  echo "           2.1.2. Not available: report an error to the standard output and finish"
  echo "   Post-processing includes, but is not limited by: "
  echo "      * de-tokenization - removing unnecessry whitespaces"
  echo "      * de-lowercasing  - restoring the capital letters in the wext where needed"
  echo "      * restoring structure - making the translated text to have structure similar"
  echo "                              to that of the original. Note that the script might,"
  echo "                              but does not have to, want to have access to the original"
  echo "                              text(s) of the pre-processor job, used to be stored as:"
  echo "              				     <work-dir>/<job-uid>.pre.in.txt"
  echo "              				     <work-dir>/<job-uid>.pre.out.txt"
  echo "              				  If not backup is done by the pre-processor script then"
  echo "                              these files are already deleted by the bpbd-processor"
  echo "                              and not available by itself. Also note that, the pre-"
  echo "                              processor server, used by the client, does not have to"
  echo "                              be the same as the server used for post-processoring."
  echo "   In addition to mentioned above, the output of this script must be two fold:"
  echo "      * If the post-processing went without errors:"
  echo "         - The exit code must be 0"
  echo "         - The only data written to standard output must be the <language>"
  echo "         - The post-processed text must be written into the file:"
  echo "              <work-dir>/<job-uid>.post.out.txt"
  echo "      * If the post-processing failed:"
  echo "         - The exit code must be 1"
  echo "         - The only data written to standard output must be the error message,"
  echo "           max 1023 characters long"
  echo "------"
  echo "NOTES:"
  echo "------"
  echo "   This is a dummy script that only emulates the real behavior, therefore:"
  echo "      * The script only copies the input file into the output."
}

#Get this script actual location to find utility scripts
SCRIPT=$(readlink "${0}")
BASEDIR=$(dirname "${SCRIPT}")

#Include the utils
. ${BASEDIR}/process_utils.sh

#Define the script type
export SCRIPT_TYPE="post"

#Process the script parameters
. ${BASEDIR}/process_params.sh

#Copy the input file into the output file
cat ${INPUT_FILE} > ${OUTPUT_FILE}

#NOTE: In case the data is needed for the post-processor script -
#move the input file to a new name and copy the output file, as
#it is still to be read by the server, thus no move.
#mv  ${INPUT_FILE} ${INPUT_FILE}.backup
#cp  ${OUTPUT_FILE} ${OUTPUT_FILE}.backup

#Output the "detected" language
echo ${LANGUAGE}
