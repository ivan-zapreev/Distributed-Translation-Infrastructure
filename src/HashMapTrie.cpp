/* 
 * File:   HashMapTrie.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 11:42 AM
 */

#include "HashMapTrie.hpp"
#include "BasicLogger.hpp"
#include <iostream>

template<unsigned int N> HashMapTrie<N>::HashMapTrie() {
}

//ToDo: This implementation is a little unflexible and ugly :) It could be
//optimized to be made recursive and dynamic but this is extra work, for 
//now this implementation is fine, after all it is just debugging
template<unsigned int N> void HashMapTrie<N>::printDebugNGram(vector<string> &tokens, const int idx, const int n ) {
        switch(n) {
            case 1:
                BasicLogger::printDebug("Adding 1-gram: [%s]", tokens[idx].c_str());
                break;
            case 2:
                BasicLogger::printDebug("Adding 2-gram: [%s %s]", tokens[idx].c_str(), tokens[idx+1].c_str());
                break;
            case 3:
                BasicLogger::printDebug("Adding 3-gram: [%s %s %s]", tokens[idx].c_str(), tokens[idx+1].c_str(), tokens[idx+2].c_str());
                break;
            case 4:
                BasicLogger::printDebug("Adding 4-gram: [%s %s %s %s]", tokens[idx].c_str(), tokens[idx+1].c_str(), tokens[idx+2].c_str(), tokens[idx+3].c_str());
                break;
            case 5:
                BasicLogger::printDebug("Adding 5-gram: [%s %s %s %s %s]", tokens[idx].c_str(), tokens[idx+1].c_str(), tokens[idx+2].c_str(), tokens[idx+3].c_str(), tokens[idx+4].c_str());
                break;
            default:
                break;
        }
}

template<unsigned int N> void HashMapTrie<N>::addWords(vector<string> &tokens) {
    //Add the words to the trie and update frequencies;
    for(vector<string>::iterator it = tokens.begin(); it != tokens.end(); ++it){
        //Insert a new or get an existing entry
        const string & token = *it;
        TWordHashSize hash = computeHash(token);
        TWordEntryPair & entry = words[hash];
        if( entry.second == 0 ) {
            entry.first = token;
        } else {
            if( entry.first.compare(token) ) {
                BasicLogger::printError("Hash collision: \'%s\' and \'%s\' both have hash %d ", token.c_str(), entry.first.c_str(), hash);
            }
        }
        //Update/increase the frequency
        entry.second++;
    }
}

template<unsigned int N> void HashMapTrie<N>::addNGram(vector<string> &tokens, const int base_idx, const int n ) {
    if( BasicLogger::getLoggingLevel() == BasicLogger::DEBUG ) {
        printDebugNGram(tokens, base_idx, n);
    }
    
    TReferenceHashSize context = computeHash(tokens[base_idx]);
    
    //Put the Ngrams into the trie with N >= 2
    for(int idx=1; idx < n; idx++) {
        string & token = tokens[base_idx+idx];
        TWordHashSize wordHash = computeHash(token);
        
        //Data stores the N-tires from length 2 on, therefore "idx-1"
        //Get/Create the mapping for this word in the Trie level of the N-gram
        TNTrieEntryPair& ngamEntry = data[idx-1][wordHash];
        //Get/Create the new entry in the map, with context
        TFrequencySize& ngramFreq = ngamEntry[context];
        
        //If this is the end of this N-gram
        if( idx == n-1) {
            //Increase the frequency of the N-gram
            ngramFreq++;
        } else {
            //Otherwise compute the next context
            context = createContext(wordHash,context);
        }
    }
}


template<unsigned int N> HashMapTrie<N>::HashMapTrie(const HashMapTrie& orig) {
}

template<unsigned int N> HashMapTrie<N>::~HashMapTrie() {
}

//Make sure that there will be templates instantiated, at least for the given parameter values
template class HashMapTrie<1>;
template class HashMapTrie<2>;
template class HashMapTrie<3>;
template class HashMapTrie<4>;
template class HashMapTrie<5>;
template class HashMapTrie<6>;
