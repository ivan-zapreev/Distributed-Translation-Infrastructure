/* 
 * File:   NGramBuilder.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 12:02 PM
 */
#include "NGramBuilder.hpp"

#include <sstream>
#include <algorithm>

#include "BasicLogger.hpp"
#include "Globals.hpp"

namespace tries {
namespace ngrams {
    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::NGramBuilder(ATrie<N,doCache> & trie, const char delim) : _trie(trie), _delim(delim) {
    }

    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::NGramBuilder(const NGramBuilder<N,doCache>& orig) : _trie(orig._trie), _delim(orig._delim) {
    }

    template<TTrieSize N, bool doCache>
    NGramBuilder<N,doCache>::~NGramBuilder() {
    }

    template<TTrieSize N, bool doCache>
    void NGramBuilder<N,doCache>::processString(const string & data ) {
        //Tokenise the line of text into a vector first
        vector<string> tokens;
        tokenize(data, _delim, tokens);

        //First add all the words to the trie
        _trie.addWords(tokens);

        //Create and record all of the N-grams starting from 2 and 
        //limited either by Trie or by the available number of Tokens
        const TTrieSize ngLevel = min<unsigned int>(_trie.getNGramLevel(), tokens.size());
        BasicLogger::printDebug("N-gram level = %u", ngLevel);
        for(int n=2; n <= ngLevel; n++) {
            for(int idx=0; idx <= (tokens.size() - n); idx++){
                BasicLogger::printDebug("adding N-grams (#tokens=%u) idx = %u, len = %u", tokens.size(), idx, n);
                _trie.addNGram(tokens, idx, n );
            }
        }
    }

    template<TTrieSize N, bool doCache>
    void NGramBuilder<N,doCache>::tokenize(const std::string &data, const char delim, vector<string> & elems) {
        stringstream stream(data);
        string token;

        //Clear the array first
        result.clear();

        //Read from the string stream
        while(getline(stream, token, delim)) {
            elems.push_back(token);
        }
    }
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class NGramBuilder<N_GRAM_PARAM,true>;
    template class NGramBuilder<N_GRAM_PARAM,false>;
}
}