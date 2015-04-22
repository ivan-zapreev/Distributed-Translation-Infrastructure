/* 
 * File:   HashMapTrie.cpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on April 18, 2015, 11:42 AM
 */
#include "HashMapTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm>      //std::fill

#include "BasicLogger.hpp"

namespace tries {
    
    template<TTrieSize N, bool doCache>
    const TTrieSize HashMapTrie<N,doCache>::MINIMUM_CONTEXT_LEVEL=2;
    
    template<TTrieSize N, bool doCache>
    HashMapTrie<N,doCache>::HashMapTrie() {
    }

    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::printDebugNGram(vector<string> &tokens, const int idx, const int n ) {
        stringstream message;
        message << "Adding " << n << "-gram: [ ";
        for(int i=idx;i<(idx+n);i++){
            message << tokens[i] << " ";
        }
        message << "]";
        BasicLogger::printDebugSafe(message.str());
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
                    stringstream message;
                    message << "Hash collision: '" << token << "' and '" << entry.first << "' both have hash " << hash;
                    BasicLogger::printErrorSafe(message.str());
                }
            }
            //Update/increase the frequency
            entry.second++;
            BasicLogger::printDebug("freq( %u ) = %u" , hash, entry.second);
        }
    }

    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::addNGram(vector<string> &tokens, const int base_idx, const int n ) {
        if( BasicLogger::getLoggingLevel() == BasicLogger::DEBUG ) {
            printDebugNGram(tokens, base_idx, n);
        }

        TReferenceHashSize context = computeHash(tokens[base_idx]);

        //Put the N-grams into the trie with N >= 2
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
                BasicLogger::printDebug("%u-gram: freq( %u, %u ) = %u", n, wordHash, context, ngramFreq);
                //BasicLogger::printDebug("data[%u][%u][%u] = %u", idx-1, wordHash, context, data[idx-1][wordHash][context]);
            } else {
                //Otherwise compute the next context
                TReferenceHashSize n_context = createContext(wordHash,context);
                BasicLogger::printDebug("%u-gram: Cn( %u, %u ) = %u", n, wordHash,context, n_context);
                context = n_context;
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
                //Now go through all of the N-grams ending with
                //the given word and sum-up their frequencies
                TNTrieEntryPairsMap & entry = data[idx-1].at(hash);
                for( auto it = entry.cbegin(); it != entry.cend(); ++it){
                    wrap.result[idx] += it->second;
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
    void HashMapTrie<N,doCache>::queryNGramFreqs(const TWordHashSize endWordHash, const TTrieSize L,
                                                 const vector<string> & ngram, vector<TWordHashSize> & hashes,
                                                 SFrequencyResult<N> & freqs) const {
        BasicLogger::printDebug(">> End word hash: %u, level %u", endWordHash, L );
        //Get this level's mapping corresponding to the start word
        const TNTrieEntryPairsMap & entry = data[L-MINIMUM_CONTEXT_LEVEL].at(endWordHash);

        BasicLogger::printDebug("-- The level %u data entry is found", L );
        
        //Compute the hash of the first word in the Ngram
        TWordHashSize startWordHash = computeHash(ngram[N-L]);
        //Put it inside the hash
        hashes.push_back(startWordHash);

        BasicLogger::printDebug("-- Computed the level's %u next %u-gram level word hash %u ", L, N-L, startWordHash );
        
        //Compute the context of the startWord on this level for this level's N-gram (L-gram))
        TReferenceHashSize context = createContext( hashes, L );

        BasicLogger::printDebug("-- The level %u context is %u", L, context );

        //BasicLogger::printDebug("Getting data[%u][%u][%u] = %u", L-1, endWordHash, context, data[L-MINIMUM_CONTEXT_LEVEL][endWordHash][context]);
        
        //Get the L-gram's frequency
        freqs.result[N-L] = entry.at(context);

        BasicLogger::printDebug("-- The level %u frequency %u is found and stored at index %u", context, freqs.result[N-L], N-L );
        
        //In case the maximum level N is not reached, do recursion
        if(L < N) {
            queryNGramFreqs(endWordHash, L+1, ngram, hashes, freqs);
        }
        BasicLogger::printDebug("<< End word hash: %u, level %u", endWordHash, L );
    }

    template<TTrieSize N, bool doCache>
    void HashMapTrie<N,doCache>::queryNGramFreqs( const vector<string> & ngram, SFrequencyResult<N> & freqs ) {
        //First just clean the array
        fill(freqs.result, freqs.result+N, 0);
        //This vector will store the N-gram's word hashes
        //for being re-used in the recursive calls
        vector<TWordHashSize> hashes;
        
        try{
            //Compute the hash of the last word in the Ngram
            TWordHashSize endWordHash = computeHash(ngram[N-1]);

            //Get the last 1-gram's word frequency
            freqs.result[N-1] = words.at(endWordHash).second;

            //Now perform a recursive procedure for finding
            //frequencies of all longer N-grams with N >= 2
            queryNGramFreqs(endWordHash, MINIMUM_CONTEXT_LEVEL, ngram, hashes, freqs);
        } catch ( out_of_range e ) {
            //DO NOTHING! This just means that the NGram is not found so we can simply stop searching
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