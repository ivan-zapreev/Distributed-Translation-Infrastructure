#!/usr/bin/env python
# coding=utf-8
# Originator:  Ondrej Dusek
# Adapted by: Dr. Ivan S. Zapreev
# Purpose: 
#   Post-process text in some languages such as:
#       'english', 'french', 'spanish', 'italian', and 'czech'
#   The process inclused de-tokenization and upper casing.
# The original scrit is obtained from:
#   https://github.com/ufal/mtmonkey

"""
A simple post-process for MT, it de-tokenizes the sentences and upper cases them
trying to use the NLTK POS tagging. This script supports languages such as:
    'english', 'french', 'spanish', 'italian', and 'czech'

Command-line usage:

    ./detokenize.py [-c] [-l LANG] [-e ENCODING] [input-file output-file]
    
    -e = use the given encoding (default: UTF-8)
    -l = use rules for the given language: a full english name
    -c = capitalize the sentences, first words, NNP and NNPS tagged words
      
    If no input and output files are given, the de-tokenizer will read
    STDIN and write to STDOUT.
"""

from __future__ import unicode_literals
from regex import Regex, UNICODE, IGNORECASE

import sys
import logging
import getopt
import codecs
import nltk

reload(sys)
DEFAULT_ENCODING = 'utf-8'
sys.setdefaultencoding(DEFAULT_ENCODING)

try:
    from nltk.tokenize import word_tokenize
    from nltk.tag import pos_tag
except ImportError:
    print '[!] You need to install nltk (http://nltk.org/index.html)'
    exit(1)

class PostProcessor(object):
    """\
    A simple post-processor class.
    """

    # Moses special characters de-escaping
    ESCAPES = [('&bar;', '|'),
               ('&lt;', '<'),
               ('&gt;', '>'),
               ('&bra;', '['),
               ('&ket;', ']'),
               ('&amp;', '&')]  # should go last to prevent double de-escaping

    # Contractions for different languages
    CONTRACTIONS = {'english': r'^\p{Alpha}+(\'(ll|ve|re|[dsm])|n\'t)$',
                    'french': r'^([cjtmnsdl]|qu)\'\p{Alpha}+$',
                    'spanish': r'^[dl]\'\p{Alpha}+$',
                    'italian': r'^\p{Alpha}*(l\'\p{Alpha}+|[cv]\'è)$',
                    'czech': r'^\p{Alpha}+[-–](mail|li)$', }

    def __init__(self, options={}):
        """\
        Constructor (pre-compile all needed regexes).
        """
        # process options
        self.moses_deescape = True if options.get('moses_deescape') else False
        self.language = options.get('language', 'english')
        self.capitalize_sents = True if options.get('capitalize_sents') else False
        # compile regexes
        self.__currency_or_init_punct = Regex(r'^[\p{Sc}\(\[\{\¿\¡]+$')
        self.__noprespace_punct = Regex(r'^[\,\.\?\!\:\;\\\%\}\]\)]+$')
        self.__cjk_chars = Regex(r'[\u1100-\u11FF\u2E80-\uA4CF\uA840-\uA87F'
                                 + r'\uAC00-\uD7AF\uF900-\uFAFF\uFE30-\uFE4F'
                                 + r'\uFF65-\uFFDC]')
        self.__final_punct = Regex(r'([\.!?])([\'\"\)\]\p{Pf}\%])*$')
        # language-specific regexes
        self.__fr_prespace_punct = Regex(r'^[\?\!\:\;\\\%]$')
        self.__contract = None
        if self.language in self.CONTRACTIONS:
            self.__contract = Regex(self.CONTRACTIONS[self.language],
                                    IGNORECASE)


    def truecase(self, sentence, lang):
        """\
        True case the sentence using the NLTP POS tagging.
        It shall capitalize NNP and NNPS entries
        """
        
        #Apply POS-tagging
        tagged_sent = pos_tag(word_tokenize(sentence, language=lang))
        #Infer capitalization from POS-tags
        normalized_sent = [w.capitalize() if t in ["NNP", "NNPS"] else w for (w,t) in tagged_sent]
        #Return the result
        return ' '.join(normalized_sent)

    
    def post_process(self, text):
        """\
        Detokenize the given text using current settings.
        """

        # strip leading/trailing space
        text = text.strip()
        
        # check if we need to perform capitalization
        if self.capitalize_sents:
            #capitalize, if the sentence ends with a final punctuation
            if self.__final_punct.search(text):
                text = text[0].upper() + text[1:]
            # capitalize the rest based on the part of speach
            text = self.truecase(text, self.language);
        
        # split text
        words = text.split(' ')
        # paste text back, omitting spaces where needed 
        text = ''
        pre_spc = ' '
        quote_count = {'\'': 0, '"': 0, '`': 0}
        for pos, word in enumerate(words):
            # remove spaces in between CJK chars
            if self.__cjk_chars.match(text[-1:]) and \
                    self.__cjk_chars.match(word[:1]):
                text += word
                pre_spc = ' '
            # no space after currency and initial punctuation
            elif self.__currency_or_init_punct.match(word):
                text += pre_spc + word
                pre_spc = ''
            # no space before commas etc. (exclude some punctuation for French)
            elif self.__noprespace_punct.match(word) and \
                    (self.language != 'french' or not
                     self.__fr_prespace_punct.match(word)):
                text += word
                pre_spc = ' '
            # contractions with comma or hyphen 
            elif word in "'-–" and pos > 0 and pos < len(words) - 1 \
                    and self.__contract is not None \
                    and self.__contract.match(''.join(words[pos - 1:pos + 2])):
                text += word
                pre_spc = ''
            # handle quoting
            elif word in '\'"„“”‚‘’`':
                # detect opening and closing quotes by counting 
                # the appropriate quote types
                quote_type = word
                if quote_type in '„“”':
                    quote_type = '"'
                elif quote_type in '‚‘’':
                    quote_type = '\''
                # exceptions for true Unicode quotes in Czech & German
                if self.language in ['czech', 'german'] and word in '„‚':
                    quote_count[quote_type] = 0
                elif self.language in ['czech', 'german'] and word in '“‘':
                    quote_count[quote_type] = 1
                # special case: possessives in English ("Jones'" etc.)                    
                if self.language == 'english' and text.endswith('s'):
                    text += word
                    pre_spc = ' '
                # really a quotation mark
                else:
                    # opening quote
                    if quote_count[quote_type] % 2 == 0:
                        text += pre_spc + word
                        pre_spc = ''
                    # closing quote
                    else:
                        text += word
                        pre_spc = ' '
                    quote_count[quote_type] += 1
            # keep spaces around normal words
            else:
                text += pre_spc + word
                pre_spc = ' '
        # de-escape chars that are special to Moses
        if self.moses_deescape:
            for char, repl in self.ESCAPES:
                text = text.replace(char, repl)
        # strip leading/trailing space
        text = text.strip()
        
        return text


def display_usage():
    """\
    Display program usage information.
    """
    print >> sys.stderr, __doc__


def open_handles(filenames, encoding):
    """\
    Open given files or STDIN/STDOUT in the given encoding.
    """
    if len(filenames) == 2:
        fh_out = codecs.open(filenames[1], 'w', encoding)
    else:
        fh_out = codecs.getwriter(encoding)(sys.stdout)
    if len(filenames) >= 1:
        fh_in = codecs.open(filenames[0], 'r', encoding)
    else:
        fh_in = codecs.getreader(encoding)(sys.stdin)
    return fh_in, fh_out


def process_sentences(func, filenames, encoding):
    """\
    Stream process given files or STDIN/STDOUT with the given function
    and encoding.
    """
    fh_in, fh_out = open_handles(filenames, encoding)
    
    #Red file sentences, there is one sentence per line
    sentences = fh_in.readlines()
    
    #Process all the sentences but the last one
    for sentence in sentences[:-1]:
        print >> fh_out, func(sentence)
    
    #Process the last line but make sure we do not print it with the new line ending
    fh_out.write(func(sentences[-1]))
    
if __name__ == '__main__':
    # parse options
    opts, filenames = getopt.getopt(sys.argv[1:], 'e:hcl:')
    options = {}
    help = False
    encoding = DEFAULT_ENCODING
    for opt, arg in opts:
        if opt == '-e':
            encoding = arg
        elif opt == '-l':
            options['language'] = arg.lower();
        elif opt == '-c':
            options['capitalize_sents'] = True
        elif opt == '-h':
            help = True
    # display help
    if len(filenames) > 2 or help:
        display_usage()
        sys.exit(1)
        
    try:
        # process the input
        detok = PostProcessor(options)
        process_sentences(detok.post_process, filenames, encoding)
    except LookupError:
        print 'The NLTK post-processor does not support \'', options['language'].capitalize(), '\' language!'
        exit(1)
