/* 
 * File:   TrieBuilder.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 11:58 AM
 */

#include "TrieBuilder.hpp"
#include "BasicLogger.hpp"
#include "NGramBuilder.hpp"

#include <string>

TrieBuilder::TrieBuilder(ITrie & trie, ifstream & fstr, const char delim) : _trie(trie), _fstr(fstr),_delim(delim){
}

TrieBuilder::TrieBuilder(const TrieBuilder& orig) : _trie(orig._trie), _fstr(orig._fstr), _delim(orig._delim) {
}

TrieBuilder::~TrieBuilder() {
}

void TrieBuilder::build() {
    BasicLogger::printInfo("Starting to read the file and build the trie ...");

    //Initialize the NGram builder and give it the trie as an argument
    NGramBuilder ngBuilder(_trie,_delim);
    
    //Iterate through the file and build n-grams per line and fill in the trie
    string line;
    while( getline(_fstr, line) )
    {
        BasicLogger::printUnsafeDebug(line);
        ngBuilder.processString(line);
    }
            
    BasicLogger::printInfo("Done reading the file and building the trie.");
}


