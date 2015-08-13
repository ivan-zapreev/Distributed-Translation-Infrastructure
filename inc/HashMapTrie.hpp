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
            template<TModelLevel N>
            class HashMapTrie : public ATrie<N> {
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

                //Stores the word hash for an unknown word
                const static TWordHashSize UNDEFINED_WORD_HASH;
                //Stores the word hash for an unknown word
                const static TWordHashSize UNKNOWN_WORD_HASH;
                //Stores the minimum known word hash
                const static TWordHashSize MIN_KNOWN_WORD_HASH;

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

                //This map stores the word index, i.e. assigns each unique word a unique id
                unordered_map<string, TWordHashSize> wordIndex;

                //The map storing the One-Grams: I.e. the word indexes and the word probabilities
                unordered_map<TWordHashSize, TProbBackOffEntryPair> oGrams;

                //The array of maps map storing n-tires for n>1 and < N
                unordered_map<TWordHashSize, TMGramEntryMap > mGrams[N - 2];

                //The map storing the N-Grams, they do not have back-off values
                unordered_map<TWordHashSize, TNGramEntryMap > nGrams;

                //The internal query results cache
                unordered_map<TWordHashSize, TCacheEntry > queryCache;

                //The temporary data structure to store the N-gram query word hashes
                TWordHashSize mGramWordHashes[N];

                //Stores the last allocated word hash
                TWordHashSize nextNewWordHash;

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                HashMapTrie(const HashMapTrie& orig);

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes are obtained from the _wordHashes member
                 * variable of the class. So it must be pre-set with proper
                 * word hash values first!
                 * @param contextLength this is the length of the considered context.
                 * @return the computed probability value
                 */
                float computeLogProbability(const TModelLevel contextLength);

                /**
                 * This method converts the M-Gram tokens into hashes and stores
                 * them in an array. Note that, M is the size of the tokens array.
                 * It is not checked, for the sake of performance but is assumed
                 * that M is <= N!
                 * @param tokens the tokens to be transformed into word hashes must have size <=N
                 * @param wordHashes the out array parameter to store the hashes.
                 */
                inline void tokensToHashes(const vector<string> & tokens, TWordHashSize wordHashes[N]) {
                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - tokens.size();
                    LOG_DEBUG1 << "Computing hashes for the words of a " << tokens.size() << "-gram:" << END_LOG;
                    for (vector<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                        wordHashes[idx] = getUniqueIdHash(*it);
                        LOG_DEBUG1 << "hash('" << *it << "') = " << wordHashes[idx] << END_LOG;
                        idx++;
                    }
                }

                /**
                 * Compute the context hash for the M-Gram prefix, example:
                 * 
                 *  N = 5
                 * 
                 *   0  1  2  3  4
                 *  w1 w2 w3 w4 w5
                 * 
                 *  contextLength = 2
                 * 
                 *    0  1  2  3  4
                 *   w1 w2 w3 w4 w5
                 *          ^  ^
                 * Hash will be computed for the 3-gram prefix w3 w4.
                 * 
                 * @param contextLength the length of the context to compute
                 * @param isBackOff is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @return the computed hash context
                 */
                inline TReferenceHashSize computeHashContext(const TModelLevel contextLength, bool isBackOff) {
                    const TModelLevel mGramEndIdx = (isBackOff ? (N - 2) : (N - 1));
                    const TModelLevel eIdx = mGramEndIdx;
                    const TModelLevel bIdx = mGramEndIdx - contextLength;
                    TModelLevel idx = bIdx;

                    LOG_DEBUG3 << "Computing context hash for context length " << contextLength
                            << " for a  " << (isBackOff ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    TReferenceHashSize contextHash = mGramWordHashes[idx];
                    LOG_DEBUG3 << "Word: " << idx << " hash == initial context hash: " << contextHash << END_LOG;
                    idx++;

                    //Compute the subsequent hashes
                    for (; idx < eIdx;) {
                        contextHash = createContext(mGramWordHashes[idx], contextHash);
                        LOG_DEBUG3 << "Idx: " << idx << ", createContext(" << mGramWordHashes[idx] << ", prevContextHash) = " << contextHash << END_LOG;
                        idx++;
                    }
                    
                    LOG_DEBUG3 << "Resulting context hash for context length " << contextLength
                            << " of a  " << (isBackOff ? "back-off" : "probability")
                            << " computation is: " << contextHash << END_LOG;

                    return contextHash;
                }

                /**
                 * This recursive function allows to get the back-off weight for the current context.
                 * The N-Gram hashes are obtained from the pre-computed data memeber array _wordHashes
                 * @param contextLength the current context length
                 * @return the resulting back-off weight probability
                 */
                float getBackOffWeight(const TModelLevel contextLength);

                /**
                 * This function computes the hash context of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * @param tokens alls of the N-gram tokens
                 * @return the resulting hash of the context(w1 w2 w3)
                 */
                inline TReferenceHashSize computeHashContext(const vector<string> & tokens) {
                    //Get the start iterator
                    vector<string>::const_iterator it = tokens.begin();
                    //Get the iterator we are going to iterate until
                    const vector<string>::const_iterator end = --tokens.end();

                    TReferenceHashSize contextHash = getUniqueIdHash(*it);
                    LOG_DEBUG2 << "contextHash = computeHash('" << *it << "') = " << contextHash << END_LOG;

                    //Iterate and compute the hash:
                    for (++it; it < end; ++it) {
                        TWordHashSize wordHash = getUniqueIdHash(*it);
                        LOG_DEBUG2 << "wordHash = computeHash('" << *it << "') = " << wordHash << END_LOG;
                        contextHash = createContext(wordHash, contextHash);
                        LOG_DEBUG2 << "contextHash = createContext( wordHash, contextHash ) = " << contextHash << END_LOG;
                    }

                    return contextHash;
                }

                /**
                 * This function gets a hash for the given word word based no the stored 1-Grams.
                 * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_HASH
                 * @param str the word to hash
                 * @return the resulting hash
                 */
                inline TWordHashSize getUniqueIdHash(const string & str) {
                    try {
                        return wordIndex.at(str);
                    } catch (out_of_range e) {
                        LOG_WARNING << "Word: '" << str << "' is not known! Mapping it to: '"
                                << UNKNOWN_WORD_STR << "', hash: " << UNKNOWN_WORD_HASH << END_LOG;
                    }
                    return UNKNOWN_WORD_HASH;
                }

                /**
                 * This function creates/gets a hash for the given word.
                 * Note: The hash id will be unique!
                 * @param str the word to hash
                 * @return the resulting hash
                 */
                inline TWordHashSize createUniqueIdHash(const string & str) {
                    //First get/create an existing/new word entry from from/in the word index
                    TWordHashSize& hash = wordIndex[str];

                    if (hash == UNDEFINED_WORD_HASH) {
                        //If the word hash is not defined yet, then issue it a new hash id
                        hash = nextNewWordHash;
                        LOG_DEBUG2 << "Word: '" << str << "' is not yet, issuing it a new hash: " << hash << END_LOG;
                        nextNewWordHash++;
                    }

                    //Use the Prime numbers hashing algorithm as it outperforms djb2
                    return hash;
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
                static inline void dessolveContext(const TReferenceHashSize context, TWordHashSize &subWord, TReferenceHashSize & subContext) {
                    //Use the Szudzik algorithm as it outperforms Cantor
                    unszudzik(context, subWord, subContext);
                }
            };

            typedef HashMapTrie<MAX_NGRAM_LEVEL> TFiveHashMapTrie;
        }
    }
}
#endif	/* HASHMAPTRIE_HPP */

