/* 
 * File:   C2DHybridTrie.cpp
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
 * Created on September 1, 2015, 15:15 PM
 */
#include "server/lm/models/c2d_hybrid_trie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "common/utils/logging/logger.hpp"
#include "common/utils/string_utils.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::utils::text;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<typename WordIndexType>
                    C2DHybridTrie<WordIndexType>::C2DHybridTrie(WordIndexType & word_index,
                            const float mram_mem_factor,
                            const float ngram_mem_factor)
                    : LayeredTrieBase<C2DHybridTrie<WordIndexType>, WordIndexType, __C2DHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_unk_data(NULL), m_mgram_mem_factor(mram_mem_factor), m_ngram_mem_factor(ngram_mem_factor), m_1_gram_data(NULL) {

                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((LM_M_GRAM_LEVEL_MAX < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                        ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");
                        ASSERT_CONDITION_THROW((sizeof(uint32_t) != sizeof(word_uid)), string("Only works with a 32 bit word_uid!"));

                        //Memset the M grams reference and data arrays
                        memset(m_m_gram_alloc_ptrs, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramAllocator *));
                        memset(m_m_gram_map_ptrs, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramsMap *));
                        memset(m_m_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (m_gram_payload *));

                        //Initialize the array of counters
                        memset(m_M_gram_num_ctx_ids, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));
                        memset(m_M_gram_next_ctx_id, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));

                        //Initialize the N-gram level data
                        m_n_gram_alloc_ptr = NULL;
                        m_n_gram_map_ptr = NULL;
                    }

                    template<typename WordIndexType>
                    void C2DHybridTrie<WordIndexType>::preAllocateOGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Compute the number of words to be stored
                        const size_t num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);

                        //Pre-allocate the 1-Gram data
                        m_1_gram_data = new m_gram_payload[num_word_ids];
                        memset(m_1_gram_data, 0, num_word_ids * sizeof (m_gram_payload));


                        //Record the dummy probability and back-off values for the unknown word
                        m_unk_data = &m_1_gram_data[UNKNOWN_WORD_ID];
                        m_unk_data->m_prob = DEFAULT_UNK_WORD_LOG_PROB_WEIGHT;
                        m_unk_data->m_back = 0.0;
                    }

                    template<typename WordIndexType>
                    void C2DHybridTrie<WordIndexType>::preAllocateMGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Pre-allocate for the M-grams with 1 < M < N
                        for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                            //Get the number of elements to pre-allocate

                            //Get the number of the M-grams on this level
                            const uint num_grams = counts[idx + 1];

                            //Reserve the memory for the map
                            reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx], &m_m_gram_alloc_ptrs[idx], num_grams, "M-Grams", m_mgram_mem_factor);

                            //Get the number of M-gram indexes on this level
                            const uint num_ngram_idx = m_M_gram_num_ctx_ids[idx];

                            m_m_gram_data[idx] = new m_gram_payload[num_ngram_idx];
                            memset(m_m_gram_data[idx], 0, num_ngram_idx * sizeof (m_gram_payload));
                        }
                    }

                    template<typename WordIndexType>
                    void C2DHybridTrie<WordIndexType>::preAllocateNGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Get the number of elements to pre-allocate

                        const size_t numEntries = counts[LM_M_GRAM_LEVEL_MAX - 1];

                        //Reserve the memory for the map
                        reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr, numEntries, "N-Grams", m_ngram_mem_factor);
                    }

                    template<typename WordIndexType>
                    void C2DHybridTrie<WordIndexType>::pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Call the super class pre-allocator!
                        BASE::pre_allocate(counts);

                        //Compute and store the M-gram level sizes in terms of the number of M-gram indexes per level
                        //Also initialize the M-gram index counters, for issuing context indexes
                        for (phrase_length i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                            //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                            m_M_gram_next_ctx_id[i] = BASE::FIRST_VALID_CTX_ID;
                            //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                            m_M_gram_num_ctx_ids[i] = counts[i + 1] + BASE::FIRST_VALID_CTX_ID;
                        }

                        //Pre-allocate 0-Grams
                        preAllocateOGrams(counts);

                        //Pre-allocate M-Grams
                        preAllocateMGrams(counts);

                        //Pre-allocate N-Grams
                        preAllocateNGrams(counts);
                    }

                    template<typename WordIndexType>
                    C2DHybridTrie<WordIndexType>::~C2DHybridTrie() {
                        //Deallocate One-Grams
                        if (m_1_gram_data != NULL) {
                            delete[] m_1_gram_data;
                        }

                        //Deallocate M-Grams there are N-2 M-gram levels in the array
                        for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                            deallocate_container<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx], &m_m_gram_alloc_ptrs[idx]);
                            delete[] m_m_gram_data[idx];
                        }

                        //Deallocate N-Grams
                        deallocate_container<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr);
                    }

                    //Make sure that there will be templates instantiated, at least for the given parameter values
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, basic_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, counting_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, hashing_word_index);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptBasicWordIndex);
                    INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptCountWordIndex);
                }
            }
        }
    }
}