#!/usr/bin/env python
# coding=utf-8
# Originator:  Ondrej Dusek
# Adapted by: Dr. Ivan S. Zapreev
# Purpose: 
#   Post-process sentence in some languages such as:
#       'english', 'french', 'spanish', 'italian', and 'czech'
#   The process inclused de-tokenization and upper casing.
#   The true casing is done using NLTK POS parser or the truecaser of
#   Nils Reimers located at: https://github.com/nreimers/truecaser
#   in case the language model is available. The language model is to
#   be located in the same folder with the script and must have a name:
#      <language-name>.obj  for example english.obj
#   The original truecase project also contains the model generation script.
# The original scrit is obtained from:
#   https://github.com/ufal/mtmonkey
# Created: 10/10/16

"""
A simple post-process for MT, it de-tokenizes the sentences and upper cases them
trying to use the NLTK POS tagging. This script supports languages such as:
    'english', 'french', 'spanish', 'italian', and 'czech'
The process inclused de-tokenization and upper casing.
The true casing is done using NLTK POS parser or the truecaser of
Nils Reimers located at: https://github.com/nreimers/truecaser
in case the language model is available. The language model is to
be located in the same folder with the script and must have a name:
    <language-name>.obj  for example english.obj
The original truecase project also contains the model generation script.

Command-line usage:

    ./post_process_nltk.py [-c] [-l LANG] [-m MODELS_DIR] [-e ENCODING] [input-file output-file]
    
    -e = use the given encoding (default: UTF-8)
    -l = use rules for the given language: a full english name
    -c = capitalize sentences
    -t = true case using the truecaser, requires the model(s)
        will use the truecaser from: https://github.com/nreimers/truecaser
    -m = the folder to search the truecaser models in
      
    If no input and output files are given, the de-tokenizer will read
    STDIN and write to STDOUT.
"""

from __future__ import unicode_literals
from regex import Regex, UNICODE, IGNORECASE

import cPickle
import os
import os.path
import sys
import inspect
import logging
import getopt
import codecs

#Include the sub-folder into the system path for module import
cmd_subfolder = os.path.realpath(os.path.abspath(os.path.join(os.path.split(inspect.getfile( inspect.currentframe() ))[0],"truecase/truecaser")))
if cmd_subfolder not in sys.path:
    sys.path.insert(0, cmd_subfolder)

from Truecaser import getTrueCase

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
        self.true_case = True if options.get('true_case') else False
        # in case the true casing is requested get the models folder
        if self.true_case:
            self.models_dir = options.get('models_dir', '.')
        
        #If the sentence is to be capitalized try loading the model
        if self.true_case:
            model_file_name = self.models_dir + "/" + self.language + ".obj"
            if os.path.isfile(model_file_name):
                #Read the model file
                f = open(model_file_name, 'rb')
                self.uniDist = cPickle.load(f)
                self.backwardBiDist = cPickle.load(f)
                self.forwardBiDist = cPickle.load(f)
                self.trigramDist = cPickle.load(f)
                self.wordCasingLookup = cPickle.load(f)
                f.close()
            else:
                print "Unable to find the truecaser model for: ", self.language
                exit(1)
        
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
        #Tokenize the sentence for the given language
        tokens = word_tokenize(sentence, language=lang)

        #Check if we have a model for truecaser
        if self.true_case:
            #Infer capitalization from the model
            tokens = getTrueCase(tokens, "as-is", self.wordCasingLookup,
                                 self.uniDist, self.backwardBiDist,
                                 self.forwardBiDist, self.trigramDist)
            
        #Return the result
        return ' '.join(tokens)

    
    def post_process(self, sentence):
        """\
        Detokenize the given sentence using current settings.
        """

        # strip leading/trailing space
        sentence = sentence.strip()
        
        # check if we need to perform capitalization
        if self.capitalize_sents:
            #capitalize, if the sentence ends with a final punctuation
            if self.__final_punct.search(sentence):
                sentence = sentence[0].upper() + sentence[1:]
            # capitalize the rest based on the part of speach
            sentence = self.truecase(sentence, self.language);
        
        # split sentence
        words = sentence.split(' ')
        # paste sentence back, omitting spaces where needed 
        sentence = ''
        pre_spc = ' '
        quote_count = {'\'': 0, '"': 0, '`': 0}
        for pos, word in enumerate(words):
            # remove spaces in between CJK chars
            if self.__cjk_chars.match(sentence[-1:]) and \
                    self.__cjk_chars.match(word[:1]):
                sentence += word
                pre_spc = ' '
            # no space after currency and initial punctuation
            elif self.__currency_or_init_punct.match(word):
                sentence += pre_spc + word
                pre_spc = ''
            # no space before commas etc. (exclude some punctuation for French)
            elif self.__noprespace_punct.match(word) and \
                    (self.language != 'french' or not
                     self.__fr_prespace_punct.match(word)):
                sentence += word
                pre_spc = ' '
            # contractions with comma or hyphen 
            elif word in "'-–" and pos > 0 and pos < len(words) - 1 \
                    and self.__contract is not None \
                    and self.__contract.match(''.join(words[pos - 1:pos + 2])):
                sentence += word
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
                if self.language == 'english' and sentence.endswith('s'):
                    sentence += word
                    pre_spc = ' '
                # really a quotation mark
                else:
                    # opening quote
                    if quote_count[quote_type] % 2 == 0:
                        sentence += pre_spc + word
                        pre_spc = ''
                    # closing quote
                    else:
                        sentence += word
                        pre_spc = ' '
                    quote_count[quote_type] += 1
            # keep spaces around normal words
            else:
                sentence += pre_spc + word
                pre_spc = ' '
        # de-escape chars that are special to Moses
        if self.moses_deescape:
            for char, repl in self.ESCAPES:
                sentence = sentence.replace(char, repl)
        # strip leading/trailing space
        sentence = sentence.strip()
        
        return sentence


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
    # check on the number of arguments
    if len(sys.argv) == 0:
        display_usage()
        sys.exit(1)
    
    # parse options
    opts, filenames = getopt.getopt(sys.argv[1:], 'e:hctl:m:')
    options = {}
    help = False
    encoding = DEFAULT_ENCODING
    for opt, arg in opts:
        if opt == '-e':
            encoding = arg
        elif opt == '-l':
            options['language'] = arg.lower();
        elif opt == '-m':
            options['models_dir'] = arg;
        elif opt == '-c':
            options['capitalize_sents'] = True
        elif opt == '-t':
            options['true_case'] = True
        elif opt == '-h':
            help = True
    
    # display help
    if help:
        display_usage()
        sys.exit(1)
        
    # the number of file names it too large
    if len(filenames) > 2:
        print "Improper list of arguments: ", sys.argv[1:]
        sys.exit(1)
        
    try:
        # process the input
        detok = PostProcessor(options)
        process_sentences(detok.post_process, filenames, encoding)
    except LookupError:
        print 'The NLTK post-processor does not support \'', options['language'].capitalize(), '\' language!'
        exit(1)
