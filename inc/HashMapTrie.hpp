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

#include "ATrie.hpp"
#include "Globals.hpp"
#include "HashingUtils.hpp"
#include "Logger.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;

namespace uva {
    namespace smt {
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
            template<TModelLevel N, bool doCache>
            class HashMapTrie : public ATrie<N, doCache> {
            public:

                /**
                 * The basic class constructor
                 */
                HashMapTrie();

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ITrie
                 */
                virtual void preAllocate(uint counts[N]);
                
                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * For more details @see ITrie
                 */
                virtual void add1Gram(const SBackOffNGram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * For more details @see ITrie
                 */
                virtual void addMGram(const SBackOffNGram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * For more details @see ITrie
                 */
                virtual void addNGram(const SBackOffNGram &nGram);

                /**
                 * Does re-set the internal query cache
                 * For more details @see ITrie
                 */
                virtual void resetQueryCache() {
                    if (doCache) {
                        queryCache.clear();
                    }
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * For more details @see ITrie
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result);

                virtual ~HashMapTrie();

            private:
                //Stores the minimum context level
                static const TModelLevel MINIMUM_CONTEXT_LEVEL;

                //The entry pair to store the N-gram probability and back off
                typedef pair<TLogProbBackOff, TLogProbBackOff> TProbBackOffEntryPair;

                //A tuple storing a word and its frequency
                typedef pair<string, TProbBackOffEntryPair> TWordEntryPair;

                //The M-trie level entry for 1 < M < N, for with probability and back-off weights
                typedef unordered_map<TReferenceHashSize, TProbBackOffEntryPair> TMGramEntryMap;

                //The N-trie level entry for the highest level M-Grams, there are no back-off weights
                typedef unordered_map<TReferenceHashSize, TLogProbBackOff> TNGramEntryMap;

                //This is the cache entry type the first value is true if the caching 
                //of this result was done, the second contains the cached results.
                typedef pair<bool, SProbResult> TCacheEntry;

                //The map storing the One-Grams: I.e. the dictionary
                unordered_map<TWordHashSize, TWordEntryPair> oGrams;

                //The array of maps map storing n-tires for n>1 and < N
                unordered_map<TWordHashSize, TMGramEntryMap > mGrams[N - 2];

                //The map storing the N-Grams, they do not have back-off values
                unordered_map<TWordHashSize, TNGramEntryMap > nGrams;

                //The internal query results cache
                unordered_map<TWordHashSize, TCacheEntry > queryCache;

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                HashMapTrie(const HashMapTrie& orig);
                
                /**
                 * This function just takes the N-Gram tokens and puts them together in one string.
                 * @param tokens the tokens to put together
                 * @return the resulting string
                 */
                string ngramToString(const vector<string> &tokens);

                /**
                 * Computes the context for the given Words.
                 * For example for hashes =  [w4 w3 w2 w1] and L = 3 this method will compute
                 * context(w4, context(w3,w2))
                 * @param hashes the pre-computed word's hashes on which the context can be computed
                 * @param L the current context level, is <= the number of hash values in hashes
                 * @return the computed context for the N-gram defined by hashes and L
                 */
                static inline TReferenceHashSize createContext(vector<TWordHashSize> & hashes, const TModelLevel cLevel) throw (Exception) {
                    const TModelLevel currMaxIdx = (cLevel - MINIMUM_CONTEXT_LEVEL);
                    //Define and default initialize the context value
                    TReferenceHashSize context = hashes.at(currMaxIdx);
                    LOG_DEBUG << "initializing context = " << context << END_LOG;

                    if (currMaxIdx > 0) {
                        LOG_DEBUG << "There is more than one element to create context from!" << END_LOG;
                        //If there is more than one element we need to create a hash for then iterate
                        for (TModelLevel idx = currMaxIdx; idx > 0; idx--) {
                            LOG_DEBUG << "context( " << hashes.at(idx - 1) << ", " << context << " )" << END_LOG;
                            context = createContext(hashes.at(idx - 1), context);
                            LOG_DEBUG << "                 = " << context << END_LOG;
                        }
                    } else {
                        if (currMaxIdx < 0) {
                            throw Exception("Unable to compute context of level <=1. It does not exist!");
                        }
                    }

                    return context;
                }

                /**
                 * This function computes the hash context of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * @param tokens alls of the N-gram tokens
                 * @return the resulting hash of the context(w1 w2 w3)
                 */
                static inline TWordHashSize computeHashContext(const vector<string> & tokens) {
                    //Get the start iterator
                    vector<string>::const_iterator it = tokens.begin();
                    //Get the iterator we are going to iterate until
                    const vector<string>::const_iterator end = --tokens.end();

                    TReferenceHashSize contextHash = computeHash(*it);
                    LOG_DEBUG2 << "contextHash = computeHash('" << *it << "') = " << contextHash << END_LOG;

                    //Iterate and compute the hash:
                    for (++it; it < end; ++it) {
                        TWordHashSize wordHash = computeHash(*it);
                        LOG_DEBUG2 << "wordHash = computeHash('" << *it << "') = " << wordHash << END_LOG;
                        contextHash = createContext(wordHash, contextHash);
                        LOG_DEBUG2 << "contextHash = createContext( wordHash, contextHash ) = " << contextHash << END_LOG;
                    }

                    return contextHash;
                }

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
                static inline void dessolveContext(const TReferenceHashSize context, TWordHashSize &subWord, TReferenceHashSize &subContext) {
                    //Use the Szudzik algorithm as it outperforms Cantor
                    unszudzik(context, subWord, subContext);
                }
            };

            typedef HashMapTrie<MAX_NGRAM_LEVEL, true> TFiveCacheHashMapTrie;
            typedef HashMapTrie<MAX_NGRAM_LEVEL, false> TFiveNoCacheHashMapTrie;

        }
    }
}
#endif	/* HASHMAPTRIE_HPP */

