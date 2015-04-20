/* 
 * File:   TrieBuilder.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 11:58 AM
 */

#include "TrieBuilder.hpp"

#include <string>

#include "BasicLogger.hpp"
#include "NGramBuilder.hpp"
#include "Globals.hpp"

namespace tries {

    using ngrams::NGramBuilder;
    
    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::TrieBuilder(ATrie<N,doCache> & trie, ifstream & fstr, const char delim) : _trie(trie), _fstr(fstr),_delim(delim){
    }

    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::TrieBuilder(const TrieBuilder<N,doCache>& orig) : _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim) {
    }

    template<TTrieSize N, bool doCache>
    TrieBuilder<N,doCache>::~TrieBuilder() {
    }

    template<TTrieSize N, bool doCache>
    void TrieBuilder<N,doCache>::build() {
        BasicLogger::printDebug("Starting to read the file and build the trie ...");
        
        //Initialize the NGram builder and give it the trie as an argument
        NGramBuilder<N,doCache> ngBuilder(_trie,_delim);

        //Iterate through the file and build n-grams per line and fill in the trie
        string line;
        while( getline(_fstr, line) )
        {
            BasicLogger::printUnsafeDebug(line);
            ngBuilder.processString(line);
        }

        BasicLogger::printDebug("Done reading the file and building the trie.");
    }
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class TrieBuilder< N_GRAM_PARAM,true >;
    template class TrieBuilder< N_GRAM_PARAM,false >;
}


