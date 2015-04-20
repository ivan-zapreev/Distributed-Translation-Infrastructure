/* 
 * File:   HashMapTrie.cpp
 * Author: zapreevis
 * 
 * Created on April 18, 2015, 11:42 AM
 */
#include "HashMapTrie.hpp"

#include <iostream>
#include <stdexcept>

#include "BasicLogger.hpp"

namespace tries {
    
    template<TTrieSize N, bool doCache>
    HashMapTrie<N,doCache>::HashMapTrie() {
    }

    //ToDo: This implementation is a little unflexible and ugly :) It could be
    //optimized to be made recursive and dynamic but this is extra work, for 
    //now this implementation is fine, after all it is just debugging
    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::printDebugNGram(vector<string> &tokens, const int idx, const int n ) {
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

    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::addWords(vector<string> &tokens) {
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

    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::addNGram(vector<string> &tokens, const int base_idx, const int n ) {
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
            TNTrieEntryPairsMap& ngamEntry = data[idx-1][wordHash];
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
    
    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::queryWordFreqs( TWordHashSize hash, SFrequencyResult<N> & wrap) {
        TFrequencySize wordFreq = 0;
        //First check if the given word is present at all, i.e. consider the 1-grams
        try{
            //Set the word's frequency, 0-gram
            TWordEntryPair & entry = words.at(hash);
            wrap.result[0] = entry.second;
            
            //Set the N-gram frequencies for N > 0, once an exception occurs this
            //means the the next level's N-gram is not present, so we can stop
            //searching already as there are definitely no occurrences of this word
            //as the last one in higher level N-grams.
            for(int idx = 1; idx < N; idx++){
                TNTrieEntryPairsMap & entry = data[idx-1].at(hash);
                for( auto it = entry.cbegin(); it != entry.cend(); ++it){
                    wrap.result[idx] = it->second;
                }
            }
            
        } catch ( out_of_range e ) {
            //DO NOTHING! This just means that the NGram is not found so we can simply stop searching
        }
    }
    
    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::queryWordFreqs(const string & word, SFrequencyResult<N> & result ) throw (Exception) {
        if( HashMapTrie<N,doCache>::doesQueryCache() ) {
            throw Exception("This function is not applicable when query result caching is ON!");
        } else {
            //Convert the word into it's cache
            TWordHashSize hash = computeHash(word);
            //Compute the result
            queryWordFreqs(hash, result );
        }
    }
    
    template<TTrieSize N, bool doCache>
    SFrequencyResult<N> & HashMapTrie<N,doCache>::queryWordFreqs(const string & word ) throw (Exception) {
        if( HashMapTrie<N,doCache>::doesQueryCache() ) {
            //Convert the word into it's cache
            TWordHashSize hash = computeHash(word);
            //Get/Create the cache entry
            TCacheEntry & cache = queryCache[hash];
            //Check if the caching was done
            if( ! cache.first ) {
                //If not then compute the result, this will be automatically cached
                queryWordFreqs(hash, cache.second );
                //Set the flag to true as caching is done
                cache.first = true;
            }
            //return the cached result
            return cache.second;
        } else {
            throw Exception("This function is not applicable when query result caching is OFF!");
        }
    }

    template<TTrieSize N, bool doCache>
    HashMapTrie<N,doCache>::HashMapTrie(const HashMapTrie& orig) {
    }

    template<TTrieSize N, bool doCache>
    HashMapTrie<N,doCache>::~HashMapTrie() {
    }
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class HashMapTrie<N_GRAM_PARAM, true>;
    template class HashMapTrie<N_GRAM_PARAM, false>;
}