/* 
 * File:   HashMapTrie.hpp
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

#ifndef HASHMAPTRIE_HPP
#define	HASHMAPTRIE_HPP

#include "ATrie.hpp"
#include "Globals.hpp"

/**
 * We actually have several choices:
 * 
 * Continue to use <ext/hash_map.h> and use -Wno-deprecated to stop the warning
 * 
 * Use <tr1/unordered_map> and std::tr1::unordered_map
 * 
 * Use <unordered_map> and std::unordered_map and -std=c++0x
 * 
 * We will need to test which one runs better, it is an unordered_map for now.
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 */

#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "HashingUtils.hpp"

using namespace std;
using namespace hashing;

namespace tries {
    /**
     * This is a HashMpa based ITrie interface implementation class.
     * Note 1: This implementation uses the unsigned long for the hashes it is not optimal
     * Note 2: the unordered_map might be not as efficient as a hash_map with respect to memory usage but it is supposed to be faster
     * 
     * This implementation is chosen because it resembles the ordered array implementation from:
     *      "Faster and Smaller N -Gram Language Models"
     *      Adam Pauls Dan Klein
     *      Computer Science Division
     *      University of California, Berkeley
     * 
     * and unordered_maps showed good performance in:
     *      "Efficient in-memory data structures for n-grams indexing"
     *       Daniel Robenek, Jan Platoˇs, and V ́aclav Sn ́aˇsel
     *       Department of Computer Science, FEI, VSB – Technical University of Ostrava
     *       17. listopadu 15, 708 33, Ostrava-Poruba, Czech Republic
     *       {daniel.robenek.st, jan.platos, vaclav.snasel}@vsb.cz
     * 
     */
    template<TTrieSize N, bool doCache>
    class HashMapTrie : public ATrie<N, doCache> {
    public:

        /**
         * The basic class constructor
         */
        HashMapTrie();

        /**
         * Does not re-set the internal query cache
         * For more details @see ITrie
         */
        virtual void addWords(vector<string> &tokens);

        /**
         * Does not re-set the internal query cache
         * For more details @see ITrie
         */
        virtual void addNGram(vector<string> &tokens, const int idx, const int n );

        /**
         * Does re-set the internal query cache
         * For more details @see ITrie
         */
        virtual void resetQueryCache(){
            if(doCache) {
                queryCache.clear();
            }
        }

        /**
         * For more details @see ITrie
         */
        virtual void queryWordFreqs(const string & word,  SFrequencyResult<N> & result ) throw (Exception);

        /**
         * For more details @see ITrie
         */
        virtual SFrequencyResult<N> & queryWordFreqs(const string & word ) throw (Exception);

        virtual ~HashMapTrie();

    private:
        //A tuple storing a word and its frequency
        typedef pair<string, TFrequencySize> TWordEntryPair;

        //The N-trie level entry tuple for a word
        typedef unordered_map<TReferenceHashSize, TFrequencySize> TNTrieEntryPairsMap;
        
        //This is the cache entry type the first value is true if the caching 
        //of this result was done, the second contains the cached results.
        typedef pair<bool, SFrequencyResult<N>> TCacheEntry;

        //The map storing the dictionary
        unordered_map<TWordHashSize, TWordEntryPair> words;

        //The map storing n-tires for n>=2 and <= N
        unordered_map<TWordHashSize, TNTrieEntryPairsMap > data[N-1];

        //The internal query results cache
        unordered_map<TWordHashSize, TCacheEntry > queryCache;

        /**
         * The copy constructor, is made private as we do not intend to copy this class objects
         * @param orig the object to copy from
         */
        HashMapTrie(const HashMapTrie& orig);

        /**
         * This function has pure debug purpose, so its impl is ugly
         * This implementation delivers quite a few hash collisions!
         * This is not something I want to have at the moment as then
         * the data structure will need to be more complex. So this
         * function will not be used.
         * @param tokens the tokens to print
         * @param hash the begin of the n-gram
         * @param n the number of elements in the n-gram
         */
        void printDebugNGram(vector<string> &tokens, const int hash, const int n );

        /**
         * This function computes the result for the given word's hash query:
         * It gets the frequencies of all the stored N-grams ending with the given word
         * @param hash the hash of the word we are after
         * @param result the N-Gram frequency values array, @see RFrequencyResult for more details
         */
        void queryWordFreqs( TWordHashSize hash, SFrequencyResult<N> & result);

        
        /**
         * This function computes the hash of the word
         * @param str the word to hash
         * @return the resulting hash
         */
        static inline TWordHashSize computeHash(const string & str) {
            //Use the Prime numbers hashing algorithm as it outperforms djb2
            return computePrimesHash(str);
        }

        /**
         * Computes the N-Gram context using the previous context and the current word hash
         * @param hash the current word hash
         * @param context the previous context
         * @return the resulting context
         */
        static inline TReferenceHashSize createContext(TWordHashSize hash, TReferenceHashSize context) {
            //Use the Szudzik algorithm as it outperforms Cantor
            return szudzik(hash, context);
        }

        /**
         * This function dissolves the given Ngram context (for N>=2) into a sub-word
         * hash and a sub-context: c(w_n) is defined by hash(w_n) and c(w_(n-1))
         * @param context the given context to dissolve 
         * @param subWord the sub-work
         * @param subContext the sub-context
         */
        static inline void dessolveContext(const TReferenceHashSize context, TWordHashSize &subWord, TReferenceHashSize &subContext){
            //Use the Szudzik algorithm as it outperforms Cantor
            unszudzik(context,subWord,subContext);
        }
    };
    
    typedef HashMapTrie<N_GRAM_PARAM,true> TFiveCacheHashMapTrie;
    typedef HashMapTrie<N_GRAM_PARAM,false> TFiveNoCacheHashMapTrie;
    
}

#endif	/* HASHMAPTRIE_HPP */

