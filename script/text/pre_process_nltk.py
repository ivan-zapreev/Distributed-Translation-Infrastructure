#!/usr/bin/env python
#coding:utf-8
# Author:  Dr. Ivan S. Zapreev
# Purpose: Pre-process the source text based on NLTK utilities
#          This script requires two parameters:
#             ${1} - is the input file name with a UTF-8 text
#             ${2} - is the input file text language name (ASCII)
#          This script prints the resulted text to standard output
# Created: 09/10/16

import codecs
from sys import exit
from sys import argv

#Make sure we only work with the UTF-8 strings
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

try:
    from nltk.tokenize import sent_tokenize
    from nltk.tokenize import word_tokenize
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

#----------------------------------------------------------------------
def _pre_process_sentence(sent, lang):
    """
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
def _pre_process_text(text, lang):
    """
    This function is used to pre-process a text.
    I.e. tokenize and lowercase.
    
    @param text: the text string
    @type sent: str
    
    @param lang: the language name
    @type lang: str
    
    @return: the pre-processed text string
    """
    #Tokenize text into sentences
    sent_list = sent_tokenize(text, language=lang)

    #Iterate over sentences and tokenize them into lower-cased words
    for sent in sent_list[:-1]:
        #Pre-process sentences and output, with a new line at the end
        print _pre_process_sentence(sent, lang)

    #Pre-process the last sentence, it is to be printed with no new-line at the end
    sys.stdout.write(_pre_process_sentence(sent_list[-1], lang))
    
    
if __name__=='__main__':
    
    #Get the input file name
    input_file_name = argv[1]
    #Lowercase the language parameter
    input_language = argv[2].lower()
    
    #Open the file with UTF-8 text
    with codecs.open(input_file_name, "r", "utf-8") as file:
        #Read the file into the text variable
        text = file.read()
        try:
            #Pre-process the text
            _pre_process_text(text, input_language)
        except LookupError:
            print 'The NLTK pre-processor does not dupport \'', input_language.capitalize(), '\' language!'
            exit(1)
