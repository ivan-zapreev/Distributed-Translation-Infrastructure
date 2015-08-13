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
 * Created on August 13, 2015, 11:42 AM
 */

#ifndef SINGLEHASHMAPTRIE_HPP
#define	SINGLEHASHMAPTRIE_HPP

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

#include "AHashMapTrie.hpp"
#include "Globals.hpp"
#include "HashingUtils.hpp"
#include "Logger.hpp"
#include "StringUtils.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is a HashMpa based ITrie interface implementation class.
             * 
             * This implementation is chosen because it resembles the single hashmap implementation mentioned in:
             *      "KenLM: Faster and Smaller Language Model Queries"
             *      Kenneth Heafield
             *      Carnegie Mellon University
             *      5000 Forbes Ave
             *      Pittsburgh, PA 15213 USA
             *      heafield@cs.cmu.edu
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
            class SingleHashMapTrie : public AHashMapTrie<N> {
            public:

                /**
                 * The basic class constructor
                 */
                SingleHashMapTrie();

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

                virtual ~SingleHashMapTrie();

            private:
                //Stores all the available N-grams, note that we also use TMGramEntryMap
                //for the N-gram although there is no back-off weight! This is not optimal!
                unordered_map<TWordHashSize, TMGramEntryMap> ngRecorder[N];

                /**
                /**
                 * Allows to add a new M-Gram with  1< M <= N
                 * @param wordHash the end M-gram word hash
                 * @param contextHash the M-Gram context hash
                 * @param gram the M-gram to add
                 */
                inline void addGram(const TWordHashSize wordHash,
                        const TReferenceHashSize contextHash, const SBackOffNGram &gram) {
                    const TModelLevel levelIdx = gram.tokens.size()-1;
                    //Obtain the entry for the given N-gram
                    TProbBackOffEntryPair & pbData = ngRecorder[levelIdx][wordHash][contextHash];

                    if (pbData.first != ZERO_LOG_PROB_WEIGHT) {
                        //If they are not the same then we have a collision!
                        REPORT_COLLISION_WARNING(gram.tokens, wordHash, contextHash,
                                pbData.first, pbData.second,
                                gram.prob, gram.back_off);
                    }

                    //Store the probability and back-off values
                    pbData.first = gram.prob;
                    pbData.second = gram.back_off;

                    LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                            << pbData.first << "," << pbData.second << ") for "
                            << ngramToString(gram.tokens) << " wordHash = "
                            << wordHash << END_LOG;
                }

                /**
                 * Allows to add a new M-Gram with  1< M <= N
                 * @param gram the M-gram to add
                 */
                template<bool isCreate>
                inline void addGram(const SBackOffNGram &gram) {
                    const size_t level = gram.tokens.size();
                    LOG_DEBUG << "Adding a " << level << "-Gram '" << ngramToString(gram.tokens) << "' to the Trie" << END_LOG;

                    // 1. Compute the context hash defined by w1 w2 w3
                    TReferenceHashSize contextHash = AHashMapTrie<N>::computeHashContext(gram.tokens);

                    // 2. Compute the hash of w4
                    const string & endWord = *(--gram.tokens.end());
                    TWordHashSize wordHash = ( isCreate ? AHashMapTrie<N>::createUniqueIdHash(endWord ) : AHashMapTrie<N>::getUniqueIdHash(endWord));
                    LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                    // 3. Insert the probability data into the trie
                    addGram(wordHash, contextHash, gram);
                }

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                SingleHashMapTrie(const SingleHashMapTrie& orig);

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes are obtained from the _wordHashes member
                 * variable of the class. So it must be pre-set with proper
                 * word hash values first!
                 * @param contextLength this is the length of the considered context.
                 * @return the computed probability value
                 */
                TLogProbBackOff computeLogProbability(const TModelLevel contextLength);

                /**
                 * This recursive function allows to get the back-off weight for the current context.
                 * The N-Gram hashes are obtained from the pre-computed data memeber array _wordHashes
                 * @param contextLength the current context length
                 * @return the resulting back-off weight probability
                 */
                TLogProbBackOff getBackOffWeight(const TModelLevel contextLength);
            };

            typedef SingleHashMapTrie<MAX_NGRAM_LEVEL> TFiveSingleHashMapTrie;
        }
    }
}
#endif	/* HASHMAPTRIE_HPP */

