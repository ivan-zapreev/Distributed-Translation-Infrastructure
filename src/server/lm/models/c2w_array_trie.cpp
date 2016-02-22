/* 
 * File:   C2WArrayTrie.cpp
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
 * Created on August 25, 2015, 11:27 PM
 */
#include "server/lm/models/c2w_array_trie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<TModelLevel MAX_LEVEL, typename WordIndexType>
                    C2WArrayTrie<MAX_LEVEL, WordIndexType>::C2WArrayTrie(WordIndexType & word_index)
                    : LayeredTrieBase<C2WArrayTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2WArrayTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_unk_data(NULL), m_1_gram_data(NULL), m_n_gram_data(NULL), m_one_gram_arr_size(0) {

                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((MAX_LEVEL < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                        ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");

                        //Memset the M grams reference and data arrays
                        memset(m_m_gram_ctx_2_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TSubArrReference *));
                        memset(m_m_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TWordIdPBEntry *));

                        //Initialize the array of counters
                        memset(m_m_n_gram_num_ctx_ids, 0, BASE::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
                        memset(m_m_n_gram_next_ctx_id, 0, BASE::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
                    }

                    template<TModelLevel MAX_LEVEL, typename WordIndexType>
                    void C2WArrayTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                        //01) Pre-allocate the word index super class call
                        BASE::pre_allocate(counts);

                        //02) Compute and store the M-gram level sizes in terms of the number of M-grams per level
                        //Also initialize the M-gram index counters, for issuing context indexes
                        for (TModelLevel i = 0; i < BASE::NUM_M_N_GRAM_LEVELS; i++) {
                            //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                            m_m_n_gram_next_ctx_id[i] = BASE::FIRST_VALID_CTX_ID;
                            //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                            m_m_n_gram_num_ctx_ids[i] = counts[i + 1] + BASE::FIRST_VALID_CTX_ID;
                        }

                        //03) Pre-allocate the 1-Gram data
                        //The size of this array is made two elements larger than the number
                        //of 1-Grams is since we want to account for the word indexes that start
                        //from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                        m_one_gram_arr_size = BASE::get_word_index().get_number_of_words(counts[0]);
                        m_1_gram_data = new m_gram_payload[m_one_gram_arr_size];
                        memset(m_1_gram_data, 0, m_one_gram_arr_size * sizeof (m_gram_payload));

                        //04) Insert the unknown word data into the allocated array
                        m_unk_data = &m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                        m_unk_data->m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                        m_unk_data->m_back = ZERO_BACK_OFF_WEIGHT;

                        //05) Allocate data for the M-grams

                        //First allocate the contexts to data mappings for the 2-grams (stored under index 0)
                        //The number of contexts is the number of words in previous level 1 i.e. counts[0]
                        //Yet we know that the word index begins with 2, due to UNDEFINED and UNKNOWN word ids
                        //Therefore for the 2-gram level contexts array we add two more elements, just to simplify computations
                        m_m_gram_ctx_2_data[0] = new TSubArrReference[m_one_gram_arr_size];
                        memset(m_m_gram_ctx_2_data[0], 0, m_one_gram_arr_size * sizeof (TSubArrReference));

                        //Now also allocate the data for the 2-Grams, the number of 2-grams is m_MN_gram_size[0] 
                        m_m_gram_data[0] = new TWordIdPBEntry[m_m_n_gram_num_ctx_ids[0]];
                        memset(m_m_gram_data[0], 0, m_m_n_gram_num_ctx_ids[0] * sizeof (TWordIdPBEntry));

                        //Now the remaining elements can be added in a loop
                        for (TModelLevel i = 1; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                            //Here i is the index of the array, the corresponding M-gram
                            //level M = i + 2. The m_MN_gram_size[i-1] stores the number of elements
                            //on the previous level - the maximum number of possible contexts.
                            m_m_gram_ctx_2_data[i] = new TSubArrReference[m_m_n_gram_num_ctx_ids[i - 1]];
                            memset(m_m_gram_ctx_2_data[i], 0, m_m_n_gram_num_ctx_ids[i - 1] * sizeof (TSubArrReference));
                            //The m_MN_gram_size[i] stores the number of elements
                            //on the current level - the number of M-Grams.
                            m_m_gram_data[i] = new TWordIdPBEntry[m_m_n_gram_num_ctx_ids[i]];
                            memset(m_m_gram_data[i], 0, m_m_n_gram_num_ctx_ids[i] * sizeof (TWordIdPBEntry));
                        }

                        //06) Allocate the data for the M-Grams.
                        m_n_gram_data = new TCtxIdProbEntry[m_m_n_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR]];
                        memset(m_n_gram_data, 0, m_m_n_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR] * sizeof (TCtxIdProbEntry));
                    }

                    template<TModelLevel MAX_LEVEL, typename WordIndexType>
                    C2WArrayTrie<MAX_LEVEL, WordIndexType>::~C2WArrayTrie() {
                        //Check that the one grams were allocated, if yes then the rest must have been either
                        if (m_1_gram_data != NULL) {
                            delete[] m_1_gram_data;
                            for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                                delete[] m_m_gram_ctx_2_data[i];
                                delete[] m_m_gram_data[i];
                            }
                            delete[] m_n_gram_data;
                        }
                    }

                    //Make sure that there will be templates instantiated, at least for the given parameter values
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, BasicWordIndex);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, CountingWordIndex);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, HashingWordIndex);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, TOptBasicWordIndex);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, TOptCountWordIndex);
                }
            }
        }
    }
}