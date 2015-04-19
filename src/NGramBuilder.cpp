/* 
 * File:   NGramBuilder.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 12:02 PM
 */

#include "NGramBuilder.hpp"
#include "BasicLogger.hpp"
#include <sstream>

NGramBuilder::NGramBuilder(ITrie & trie, const char delim) : _trie(trie), _delim(delim) {
}

NGramBuilder::NGramBuilder(const NGramBuilder& orig) : _trie(orig._trie), _delim(orig._delim) {
}

NGramBuilder::~NGramBuilder() {
}

void NGramBuilder::processString(const string & data ) {
    //Tokenise the line of text into a vector first
    vector<string> tokens;
    tokenize(data, _delim, tokens);

    //First add all the words to the trie
    _trie.addWords(tokens);
    
    //Create and record all of the N-grams starting from 2.
    for(int n=2; n <= _trie.getNGramLevel(); n++) {
        for(int idx=0; idx <= tokens.size()- n; idx++){
            _trie.addNGram(tokens, idx, n );
        }
    }
}

void NGramBuilder::tokenize(const std::string &data, const char delim, vector<string> & elems) {
    stringstream stream(data);
    string token;
    
    //Clear the array first
    result.clear();
    
    //Read from the string stream
    while(getline(stream, token, delim)) {
        elems.push_back(token);
    }
}
