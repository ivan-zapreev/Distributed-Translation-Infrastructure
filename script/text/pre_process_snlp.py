#!/usr/bin/env python
#coding:utf-8
# File:    pre_process_snlp.py
# Author:  Dr. Ivan S. Zapreev
# Purpose: Pre-processor helper for the source text based on Stanford Core NLP output
#          This script requires three parameters:
#             ${1} - is the input file name with Stanford CoreNLP processed UTF-8 text
#             ${2} - is the template file name to generate the template into, optional
#             ${3} - is the input file name with UTF-8 text, optional is only needed if
#                    the template file is to be generated
#          This script prints the resulted text to standard output
#
# Visit my Linked-in profile:
#      <https://nl.linkedin.com/in/zapreevis>
# Visit my GitHub:
#      <https://github.com/ivan-zapreev>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
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

"""
A simple pre-processing helper script to handle the text output produced by
the Stanford CoreNLP sentence split and tokenized text. The main task of
this script is to transform the SNLP output format into a simple
one-tokenized-sentence-per-line format. In case the -t and -s options are
specified then the script will also generate the template file for text
structure restoration to be done in the post-processing script.

Command-line usage:

    ./pre_process_nltk.py [-h] [-t TEMPL] [-s ORIG_FILE_NAME ] [SNLP_FILE_NAME]
    
    -h = the help message
    -t = the template file name to be generated, optional, is only needed if '-s' is specified
    -s = the orginal source text file name, optional, is only needed if '-t' is specified
    
    The resulting pre-processed text will be written to STDOUT
"""

import codecs
import getopt
from sys import exit
from sys import argv
from regex import Regex, UNICODE, IGNORECASE

#Make sure we only work with the UTF-8 strings
import sys
reload(sys)
DEFAULT_ENCODING = 'utf-8'
sys.setdefaultencoding(DEFAULT_ENCODING)

#----------------------------------------------------------------------
def display_usage():
    """\
    Display program usage information.
    """
    print >> sys.stderr, __doc__


#----------------------------------------------------------------------
def output_sentence(is_first_sent, sentence):
    """\
    Allows to output the sentence to the standard output, if it is not empty
    also checks whether it is the first sentence being output or not, if not
    then it is being preceeded with a new line.
    """
    #Strip the sentence to remove unneded whitespaces
    sentence = sentence.strip()
    #If this is not the first sentence we read
    if sentence != "":
        #If this is the first sentence we dump no starting
        #new line, otherwise start at a new line.
        if is_first_sent:
            sys.stdout.write(sentence)
            is_first_sent = False
        else:
            sys.stdout.write("\n" + sentence.strip())
    #Return the first sentence flag and re-set sentence value
    return is_first_sent, ""


#----------------------------------------------------------------------
def convert_data(snlp_file, options):
    """\
    Converts the text from file and outputs it to STDOUT
    In case the source and template options are given, it
    also produces the template file
    """
    #Set the flag indicating whether we need to produce the template
    is_templ = ( options.get('is_source') & options.get('is_templ') )
    
    #In case the template is to be produced open the files
    if is_templ:
        with codecs.open( options['source_file_name'], "r", DEFAULT_ENCODING) as source_in:
            #Read the source text text
            text = source_in.read();
            
    #Read the SNLP file into lines
    lines = snlp_file.readlines();
    #Declare the Regular expression templates
    sent_reg_ex = Regex(r'^Sentence\s\#\d+\s\(\d+\stokens\)\:$')
    token_reg_ex = Regex(r'^\[Text=(.*)\sCharacterOffsetBegin=\d+\sCharacterOffsetEnd=\d+\]$')
    #Declare sentence variable for storing the tokenized sentence
    sentence = ""
    #Declare the flag indicating that this is the first sentence we output
    is_first_sent = True
    #Declare the sentence index
    idx = 0
    #Iterate the file line by line and write output
    for line in lines:
        #Chek if we have a new sentence start
        if sent_reg_ex.match(line):
            #Output the sentence there is up till now, if any
            is_first_sent, sentence = output_sentence(is_first_sent, sentence)
        #Check if we have a new token start
        else:
            #Try to match the token line
            match = token_reg_ex.match(line)
            #If we have matched the line
            if match:
                #Append the token to the sentence and add a space character
                sentence += match.group(1) + " "
            #In this case we have an original sentence for templating
            else:   
                #If the template is to be generated
                if is_templ:
                    #Replace the sentence with its placeholder
                    text = text.replace(line.strip(), "{"+str(idx)+"}", 1)
                    idx += 1
    
    #Output the last sentence that was read in the loop, if any
    is_first_sent, sentence = output_sentence(is_first_sent, sentence)
    
    #Write the template file bacl
    if is_templ:
        with codecs.open( options['templ_file_name'], "w", DEFAULT_ENCODING) as templ:
            #Output the resulting text
            templ.write(text);

#----------------------------------------------------------------------
if __name__=='__main__':
    # check on the number of arguments
    if len(argv) == 0:
        display_usage()
        exit(1)
    
    #Parse options
    opts, filenames = getopt.getopt(argv[1:], 'ht:s:')
    options = {}
    help = False
    for opt, arg in opts:
        if opt == '-s':
            options['is_source'] = True
            options['source_file_name'] = arg;
        elif opt == '-t':
            options['is_templ'] = True
            options['templ_file_name'] = arg
        elif opt == '-h':
            help = True
    
    #Display help
    if help:
        display_usage()
        exit(0)
        
    #The number of file names it too large
    if len(filenames) > 1:
        print "ERROR: Improper list of arguments: ", argv[1:]
        exit(1)
        
    #Check that we either have -s and -t or not
    if options.get('is_source') != options.get('is_templ'):
        print "ERROR: Using -s presumes -t and vise versa!"
        display_usage()
        exit(1)

    #Open the file with UTF-8 text
    with codecs.open(filenames[0], "r", DEFAULT_ENCODING) as file:
        #Call the function doing text processing
        convert_data(file, options);
