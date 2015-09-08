/* 
 * File:   G2DHashMapTrie.hpp
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
 * Created on September 3, 2015, 3:32 PM
 */

#ifndef G2DHASHMAPTRIE_HPP
#define	G2DHASHMAPTRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "ATrie.hpp"
#include "AWordIndex.hpp"

#include "TextPieceReader.hpp"

#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

using namespace std;

using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::dictionary;

using namespace __G2DHashMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __G2DHashMapTrie {
                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 */
                template<typename PAYLOAD_TYPE, typename M_GRAM_ID_TYPE>
                struct S_M_GramData {
                    M_GRAM_ID_TYPE * id;
                    PAYLOAD_TYPE data;
                };

                /**
                 * This structure represents a trie level entry bucket
                 * it stores the bucket capacity, size and the pointer to its elements.
                 */
                template<typename PAYLOAD_TYPE, typename M_GRAM_ID_TYPE>
                struct STrieBucket {
                    S_M_GramData<PAYLOAD_TYPE, M_GRAM_ID_TYPE> * grams;
                    uint8_t capacity;
                    uint8_t size;
                };
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N>
            class G2DHashMapTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit G2DHashMapTrie(AWordIndex * const _pWordIndex)
                : ATrie<N>(_pWordIndex), m_1_gram_data(NULL), m_N_gram_data(NULL) {
                    //Initialize the array of number of gram ids per level
                    memset(num_buckets, 0, N * sizeof (TShortId));

                    //Clear the M-Gram bucket arrays
                    memset(m_M_gram_data, 0, ATrie<N>::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket));

                    //Get the memory increase strategy
                    m_p_mem_strat = getMemIncreaseStrategy(__G2DHashMapTrie::MEM_INC_TYPE,
                            __G2DHashMapTrie::MIN_MEM_INC_NUM, __G2DHashMapTrie::MEM_INC_FACTOR);

                    LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO3 << "Using the " << m_p_mem_strat->getStrategyStr()
                            << "' memory allocation strategy." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see ATrie
                 */
                virtual void pre_allocate(const size_t counts[N]) {
                    //Call the base-class
                    ATrie<N>::pre_allocate(counts);

                    //02) Pre-allocate the 1-Gram data
                    num_buckets[0] = ATrie<N>::get_word_index()->get_words_count(counts[0]);
                    m_1_gram_data = new TProbBackOffEntry[num_buckets[0]];
                    memset(m_1_gram_data, 0, num_buckets[0] * sizeof (TProbBackOffEntry));

                    //03) Insert the unknown word data into the allocated array
                    TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                    pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                    pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                    //Compute the number of M-Gram level buckets and pre-allocate them
                    for (TModelLevel idx = 0; idx < ATrie<N>::NUM_M_GRAM_LEVELS; idx++) {
                        num_buckets[idx + 1] = counts[idx + 1];
                        m_M_gram_data[idx] = new TProbBackOffBucket[num_buckets[idx + 1]];
                    }

                    //Compute the number of N-Gram level buckets and pre-allocate them
                    num_buckets[N - 1] = counts[N - 1];
                    m_N_gram_data = new TProbBucket[num_buckets[N - 1]];
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @see ATrie
                 */
                virtual void add_1_gram(const T_M_Gram &oGram) {
                    //Register a new word, and the word id will be the one-gram id
                    const TShortId oneGramId = ATrie<N>::get_word_index()->register_word(oGram.tokens[0]);
                    //Store the probability data in the one gram data storage, under its id
                    m_1_gram_data[oneGramId].prob = oGram.prob;
                    m_1_gram_data[oneGramId].back_off = oGram.back_off;
                };

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see ATrie
                 */
                virtual void add_m_gram(const T_M_Gram &mGram) {
                    //Compute the hash value for the given M-gram, it must
                    //be the M-Gram id in the M-Gram data storage
                    const TShortId mGramHash = mGram.hash();
                    //Compute the bucket Id from the M-Gram hash
                    const TShortId bucketId = mGramHash % num_buckets[mGram.level];

                    //If the sanity check is on then check on that the id is within the range
                    if (DO_SANITY_CHECKS && ((bucketId < 0) || (bucketId >= num_buckets[mGram.level]))) {
                        stringstream msg;
                        msg << "The " << SSTR(mGram.level) << "-gram: " << tokensToString<N>(mGram)
                                << " was given an incorrect hash: " << SSTR(bucketId)
                                << ", must be within [0, " << SSTR(num_buckets[mGram.level]) << "]";
                        throw Exception(msg.str());
                    }

                    //ToDo: Create a new M-Gram Id (unique) and then add it to the data

                    throw Exception("add_M_Gram");
                };

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it not guaranteed that the parameter will be checked to be a N-Gram!
                 * @see ATrie
                 */
                virtual void add_n_gram(const T_M_Gram &nGram) {
                    //ToDo: Implement
                    throw Exception("add_N_Gram");
                };

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @see ATrie
                 */
                virtual void query(const T_M_Gram & ngram, TQueryResult & result) {
                    //ToDo: Implement
                    throw Exception("queryNGram");
                };

                /**
                 * The basic class destructor
                 */
                virtual ~G2DHashMapTrie() {
                    //Check that the one grams were allocated, if yes then the rest must have been either
                    if (m_1_gram_data != NULL) {
                        //De-allocate one grams
                        delete[] m_1_gram_data;
                        //De-allocate M-Grams
                        for (TModelLevel idx = 0; idx < ATrie<N>::NUM_M_GRAM_LEVELS; idx++) {
                            delete[] m_M_gram_data[idx];
                        }
                        //De-allocate N-Grams
                        delete[] m_N_gram_data;
                    }
                };

            protected:


            private:

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //These are arrays of buckets for M-Gram levels with 1 < M < N
                typedef __G2DHashMapTrie::STrieBucket<TProbBackOffEntry, T_Compressed_M_Gram_Id > TProbBackOffBucket;
                TProbBackOffBucket * m_M_gram_data[ATrie<N>::NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef __G2DHashMapTrie::STrieBucket<TLogProbBackOff, T_Compressed_M_Gram_Id > TProbBucket;
                TProbBucket * m_N_gram_data;

                //Stores the number of gram ids/buckets per level
                TShortId num_buckets[N];

                //Stores the memory increase strategy object
                MemIncreaseStrategy * m_p_mem_strat;
            };
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

