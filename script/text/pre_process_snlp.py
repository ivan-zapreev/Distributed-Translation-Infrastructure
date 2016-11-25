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
    
if __name__=='__main__':
    # check on the number of arguments
    if len(argv) == 0:
        display_usage()
        exit(1)
    
    # parse options
    opts, filenames = getopt.getopt(argv[1:], 'ht:s:')
    options = {}
    help = False
    for opt, arg in opts:
        if opt == '-s':
            options['source_file_name'] = arg;
        elif opt == '-t':
            options['is_templ'] = True
            options['templ_file_name'] = arg
        elif opt == '-h':
            help = True
    
    # display help
    if help:
        display_usage()
        exit(0)
        
    # the number of file names it too large
    if len(filenames) > 1:
        print "ERROR: Improper list of arguments: ", argv[1:]
        exit(1)
    
    #Check that the language is defined, it is compulsory
    if not 'language' in options:
        print "ERROR: Please specify the input file's language"
        display_usage()
        exit(1)

    #ToDo process text