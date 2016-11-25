#!/usr/bin/env python
#coding:utf-8
# File:    pre_process_nltk.py
# Author:  Dr. Ivan S. Zapreev
# Purpose: Pre-process the source text based on NLTK utilities
#          This script requires two parameters:
#             ${1} - is the input file name with a UTF-8 text
#             ${2} - is the input file text language name (ASCII)
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
A simple pre-process for MT, it tokenizes text into sentences and sentences
into words and lower-cases them. The script is NLTK based and thus supports
the same list of languages. This script performs the language detection based
on the stop words analysis.

Command-line usage:

    ./pre_process_nltk.py [-h] [-l LANG] [-t TEMPL] [input-file]
    
    -h = the help message
    -l = the english name of the language in which the text of the input file is written
    -t = the name of the template file to be generated, optional
    
    The pre-processed text will be written to STDOUT
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

try:
    from nltk.tokenize import sent_tokenize
    from nltk.tokenize import word_tokenize
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

#----------------------------------------------------------------------
def display_usage():
    """\
    Display program usage information.
    """
    print >> sys.stderr, __doc__

#----------------------------------------------------------------------
def _pre_process_sentence(sent, lang):
    """\
    This function is used to pre-process a single sentence.
    I.e. tokenize and lowercase.
    
    @param sent: the sentence string
    @type sent: str
    
    @param lang: the language name
    @type lang: str
    
    @return: the pre-processed sentence string
    """
    #Tokenize into words
    tokens = word_tokenize(sent, language=lang)
    #Lowercase the words
    words = [word.lower() for word in tokens]
    #Return the space-separated tokens
    return u" ".join(words)

#----------------------------------------------------------------------
def _pre_process_text(text, options):
    """\
    This function is used to pre-process a text.
    I.e. tokenize and lowercase.
    
    @param text: the text string
    @type text: str
    
    @param options: tje parameters dictionary
    @type options: dictionary
    
    @return: the pre-processed text string
    """
    
    #Get the language
    lang = options.get('language')
    
    #Tokenize text into sentences
    sent_list = sent_tokenize(text, language=lang)

    #Check if we need to generate a template
    if options.get('is_templ'):
        #Declare the sentence index
        idx=0
        #Iterate over sentences and tokenize them into lower-cased words
        #In parallel generate the text template and put it into a file
        for sent in sent_list[:-1]:
            #Replace the sentence with its placeholder
            text = text.replace(sent.strip(), "{"+str(idx)+"}", 1)
            idx +=1
            #Pre-process sentences and output, with a new line at the end
            print _pre_process_sentence(sent, lang)
        
        #Get the last sentence
        last_sent = sent_list[-1]
        #Replace the last sentence with its placeholder
        text = text.replace(last_sent.strip(), "{"+str(idx)+"}", 1)
        #Pre-process the last sentence, it is to be printed with no new-line at the end
        sys.stdout.write(_pre_process_sentence(last_sent, lang))
            
        #Open the template file for writing
        with codecs.open(options['templ_file_name'], "w", DEFAULT_ENCODING) as templ:
            #Output the resulting text
            templ.write(text);
    else:
        #Iterate over sentences and tokenize them into lower-cased words
        for sent in sent_list[:-1]:
            #Pre-process sentences and output, with a new line at the end
            print _pre_process_sentence(sent, lang)

        #Pre-process the last sentence, it is to be printed with no new-line at the end
        sys.stdout.write(_pre_process_sentence(sent_list[-1], lang))
    
if __name__=='__main__':
    # check on the number of arguments
    if len(argv) == 0:
        display_usage()
        exit(1)
    
    # parse options
    opts, filenames = getopt.getopt(argv[1:], 'hl:t:')
    options = {}
    help = False
    for opt, arg in opts:
        if opt == '-l':
            options['language'] = arg.lower();
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
    
    #Open the file with UTF-8 text
    with codecs.open(filenames[0], "r", DEFAULT_ENCODING) as file:
        #Read the file into the text variable
        text = file.read()
        try:
            #Pre-process the text
            _pre_process_text(text, options)
        except LookupError:
            print 'ERROR: The NLTK pre-processor does not support \'', options['language'].capitalize(), '\' language!'
            exit(1)
