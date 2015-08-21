/* 
 * File:   SingleHashMapTrie.hpp
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

#ifndef AHASHMAPTRIE_HPP
#define	AHASHMAPTRIE_HPP

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
#include <inttypes.h>     // uint8_t

#include "ATrie.hpp"
#include "Globals.hpp"
#include "HashingUtils.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {

            //This macro is needed to report the collision detection warnings!
#define REPORT_COLLISION_WARNING(N, gram, wordHash, contextHash, prevProb, prevBackOff, newProb, newBackOff)    \
            LOG_WARNING << "The " << gram.level << "-Gram : '" << tokensToString<N>(gram.tokens, gram.level)    \
                        << "' has been already seen! "  << "wordHash: " << SSTR(wordHash)                    \
                        << ", contextHash: " << SSTR(contextHash) << ". "                                    \
                        << "Changing the (prob,back-off) data from ("                                        \
                        << prevProb << "," << prevBackOff << ") to ("                                        \
                        << newProb << "," << newBackOff << ")" << END_LOG;

            //The entry pair to store the N-gram probability and back off
            typedef pair<TLogProbBackOff, TLogProbBackOff> TProbBackOffEntryPair;

            //The 2-trie level entry for 1 < 2 < N, for with probability and back-
            //off weights. The difference here compared to the TMGramEntryMap is
            //that we can use TWordHashSize data type instead of TReferenceHashSize
            //for the key values, as the context is always the first word's hash.
            typedef unordered_map<TWordHashSize, TProbBackOffEntryPair> TBGramEntryMap;

            //The M-trie level entry for 1 < M < N, for with probability and back-off weights
            typedef unordered_map<TReferenceHashSize, TProbBackOffEntryPair> TMGramEntryMap;

            //The N-trie level entry for the highest level M-Grams, there are no back-off weights
            typedef unordered_map<TReferenceHashSize, TLogProbBackOff> TNGramEntryMap;

            /**
             * This is a base abstract class for the Trie implementation using hash tables.
             * The class only contains a few basic features, such as hashing functions and
             * methods for working with the queued M-gram.
             */
            template<TModelLevel N>
            class AHashMapTrie : public ATrie<N> {
            public:

                /**
                 * The basic class constructor, accepts memory factor(s) that are the
                 * coefficients used when pre-allocating memory for unordered maps.
                 * 
                 * If a factor is equal to 0.0 then no memory is pre-allocated.
                 * If the factor is equal to 1.0 then there is only as much preallocated
                 * as needed to store the gram entries. The latter is typically not enough
                 * as unordered_map needs more memory for internal administration.
                 * If there is not enough memory pre-allocated then additional allocations
                 * will take place but it does not alway lead to more efficient memory
                 * usage. The observed behavior is that it is better to pre-allocate
                 * a bit more memory beforehand, than needed. This leads to less
                 * memory consumption. Depending on the type of unordered_map
                 * key/value pair types the advised factor values are from 2.0 to 2.6.
                 * Because it can not be optimally determined beforehand, these are made
                 * constructor parameters so that they can be configured by the used.
                 * This breaks encapsulation a bit, exposing the internals, but
                 * there is no other better way, for fine tuning the memory usage.
                 * 
                 * @param _wordIndex the word index to be used
                 */
                explicit AHashMapTrie( AWordIndex * const _pWordIndex) : ATrie<N>(_pWordIndex) {
                };

                /**
                 * The defaul implementation that pre-allocates the wordIndex
                 * @param counts the number of ngrams
                 */
                virtual void preAllocate(const size_t counts[N]) {
                    //Compute the number of words to be stored
                    //Add an extra element for the <unknown/> word
                    ATrie<N>::pWordIndex->preAllocate(counts[0] + 1);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~AHashMapTrie() {
                };

            protected:

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                AHashMapTrie(const AHashMapTrie& orig) {
                    throw Exception("AHashMapTrie copy constructor is not to be used, unless implemented!");
                };

                /**
                 * Gets the word hash for the end word of the back-off N-Gram
                 * @return the word hash for the end word of the back-off N-Gram
                 */
                inline const TWordHashSize & getBackOffNGramEndWordHash() {
                    return mGramWordHashes[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the N-gram
                 * @return the word hash for the last word in the N-gram
                 */
                inline const TWordHashSize & getNGramEndWordHash() {
                    return mGramWordHashes[N - 1];
                }

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
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(tokens.size()) << "-gram:" << END_LOG;
                    for (vector<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                        wordHashes[idx] = ATrie<N>::pWordIndex->getUniqueIdHash(*it);
                        LOG_DEBUG1 << "hash('" << *it << "') = " << SSTR(wordHashes[idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * Converts the given tokens to hashes and stores it in mGramWordHashes
                 * @param ngram the n-gram tokens to convert to hashes
                 */
                inline void storeNGramHashes(const vector<string> & ngram) {
                    //First transform the given M-gram into word hashes.
                    tokensToHashes(ngram, mGramWordHashes);
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

                    LOG_DEBUG3 << "Computing context hash for context length " << SSTR(contextLength)
                            << " for a  " << (isBackOff ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    TReferenceHashSize contextHash = mGramWordHashes[idx];
                    LOG_DEBUG3 << "Word: " << SSTR(idx) << " hash == initial context hash: " << SSTR(contextHash) << END_LOG;
                    idx++;

                    //Compute the subsequent hashes
                    for (; idx < eIdx;) {
                        contextHash = createContext(mGramWordHashes[idx], contextHash);
                        LOG_DEBUG3 << "Idx: " << SSTR(idx) << ", createContext(" << SSTR(mGramWordHashes[idx]) << ", prevContextHash) = " << SSTR(contextHash) << END_LOG;
                        idx++;
                    }

                    LOG_DEBUG3 << "Resulting context hash for context length " << SSTR(contextLength)
                            << " of a  " << (isBackOff ? "back-off" : "probability")
                            << " computation is: " << SSTR(contextHash) << END_LOG;

                    return contextHash;
                }

                /**
                 * This function computes the hash context of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * @param gram the N-gram with its tokens to create context for
                 * @return the resulting hash of the context(w1 w2 w3) or UNDEFINED_WORD_HASH for any M-Gram with M <= 1
                 */
                template<DebugLevel logLevel>
                inline TReferenceHashSize computeHashContext(const SRawNGram & gram) {
                    TReferenceHashSize contextHash = UNDEFINED_WORD_HASH;

                    //If it is more than a 1-Gram then compute the context, otherwise it is undefined.
                    if (gram.level > MIN_NGRAM_LEVEL) {
                        //Get the start context value for the first token
                        const string & token = gram.tokens[0].str();
                        contextHash = ATrie<N>::pWordIndex->getUniqueIdHash(token);

                        LOGGER(logLevel) << "contextHash = computeHash('" << token << "') = " << SSTR(contextHash) << END_LOG;

                        //Iterate and compute the hash:
                        for (int i = 1; i < (gram.level - 1); i++) {
                            const string & token = gram.tokens[i].str();
                            TWordHashSize wordHash = ATrie<N>::pWordIndex->getUniqueIdHash(token);
                            LOGGER(logLevel) << "wordHash = computeHash('" << token << "') = " << SSTR(wordHash) << END_LOG;
                            contextHash = createContext(wordHash, contextHash);
                            LOGGER(logLevel) << "contextHash = createContext( wordHash, contextHash ) = " << SSTR(contextHash) << END_LOG;
                        }
                    }

                    return contextHash;
                }

                /**
                 * Computes the N-Gram context using the previous context and the current word hash
                 * @param hash the current word hash
                 * @param context the previous context
                 * @return the resulting context
                 */
                static inline TReferenceHashSize createContext(TWordHashSize hash, TReferenceHashSize context) {
                    //Use the Szudzik algorithm as it outperforms Cantor
                    return cantor(hash, context);
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

                void recordAndCheck(const TWordHashSize wordHash,
                        const TReferenceHashSize contextHash, const SNiceNGram &gram) const {
                }

            private:

                //The temporary data structure to store the N-gram query word hashes
                TWordHashSize mGramWordHashes[N];

            };
        }
    }
}
#endif	/* HASHMAPTRIE_HPP */

