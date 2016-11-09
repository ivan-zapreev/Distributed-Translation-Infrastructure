#!/usr/bin/env python
#coding:utf-8
# Author:  Dr. Ivan S. Zapreev
# Purpose: Detecting language using a stopwords based approach
# Derived: From http://blog.alejandronolla.com/2013/05/15/detecting-text-language-with-python-and-nltk/
# Created: 09/10/16

import codecs
from sys import exit
from sys import argv
from itertools import islice

try:
    from nltk import wordpunct_tokenize
    from nltk.corpus import stopwords
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

#----------------------------------------------------------------------
def _calculate_languages_ratios(text):
    """
    Calculate probability of given text to be written in several languages and
    return a dictionary that looks like {'french': 2, 'spanish': 4, 'english': 0}
    
    @param text: Text whose language want to be detected
    @type text: str
    
    @return: Dictionary with languages and unique stopwords seen in analyzed text
    @rtype: dict
    """

    languages_ratios = {}

    '''
    nltk.wordpunct_tokenize() splits all punctuations into separate tokens
    
    >>> wordpunct_tokenize("That's thirty minutes away. I'll be there in ten.")
    ['That', "'", 's', 'thirty', 'minutes', 'away', '.', 'I', "'", 'll', 'be', 'there', 'in', 'ten', '.']
    '''

    tokens = wordpunct_tokenize(text)
    words = [word.lower() for word in tokens]

    # Compute per language included in nltk number of unique stopwords appearing in analyzed text
    for language in stopwords.fileids():
        stopwords_set = set(stopwords.words(language))
        words_set = set(words)
        common_elements = words_set.intersection(stopwords_set)

        languages_ratios[language] = len(common_elements) # language "score"

    return languages_ratios


#----------------------------------------------------------------------
def detect_language(text):
    """
    Calculate probability of given text to be written in several languages and
    return the highest scored. In case the language can not be detected the
    script exits with an error code.
    
    It uses a stopwords based approach, counting how many unique stopwords
    are seen in analyzed text.
    
    @param text: Text whose language want to be detected
    @type text: str
    
    @return: Most scored language guessed
    @rtype: str
    """

    ratios = _calculate_languages_ratios(text)

    most_rated_language = max(ratios, key=ratios.get)
    
    #Check if the language was indeed dected
    if ratios[most_rated_language] == 0:
        print 'Could not detect the source language'
        exit(1)

    return most_rated_language


if __name__=='__main__':

    """
    Only read a small buffer of 1024 UTF-8 characters to do
    language detection, no need to do it on an entire file.
    """
    text = ''
    with codecs.open(argv[1], "r", "utf-8") as file:
        text = file.read(1024)
    
    language = detect_language(text)

    #Return the detected language capitalizing the first letter
    print language[0].capitalize() + language[1:]
