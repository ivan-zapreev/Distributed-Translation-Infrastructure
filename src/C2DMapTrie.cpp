/* 
 * File:   C2DMapTrie.cpp
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
 * Created on August 14, 2015, 1:53 PM
 */
#include "C2DMapTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "Logger.hpp"
#include "StringUtils.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            C2DMapTrie<MAX_LEVEL, WordIndexType>::C2DMapTrie(
                    WordIndexType & word_index,
                    const float mgram_mem_factor,
                    const float ngram_mem_factor)
            : LayeredTrieBase<C2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DMapTrie::DO_BITMAP_HASH_CACHE>(word_index),
            m_mgram_mem_factor(mgram_mem_factor),
            m_ngram_mem_factor(ngram_mem_factor),
            m_1_gram_data(NULL) {
                //Perform sanity checks
                if (DO_SANITY_CHECKS) {
                    //Initialize the hash statistics map
                    for (int i = 0; i < MAX_LEVEL; i++) {
                        m_hash_sizes[i].first = UINT64_MAX;
                        m_hash_sizes[i].second = 0;
                    }
                }

                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate_1_grams(const size_t counts[MAX_LEVEL]) {
                //Compute the number of words to be stored
                const size_t num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);

                //Pre-allocate the 1-Gram data
                m_1_gram_data = new T_M_Gram_Payload[num_word_ids];
                memset(m_1_gram_data, 0, num_word_ids * sizeof (T_M_Gram_Payload));


                //Record the dummy probability and back-off values for the unknown word
                T_M_Gram_Payload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.m_back = ZERO_BACK_OFF_WEIGHT;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate_m_grams(const size_t counts[MAX_LEVEL]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 1; idx < (MAX_LEVEL - 1); idx++) {
                    //Get the number of elements to pre-allocate

                    const uint numEntries = counts[idx];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx - 1], &m_m_gram_alloc_ptrs[idx - 1], numEntries, "M-Grams", m_mgram_mem_factor);
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate_n_grams(const size_t counts[MAX_LEVEL]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[MAX_LEVEL - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr, numEntries, "N-Grams", m_ngram_mem_factor);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void C2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the super class pre-allocator!

                BASE::pre_allocate(counts);

                //Pre-allocate 0-Grams
                pre_allocate_1_grams(counts);

                //Pre-allocate M-Grams
                pre_allocate_m_grams(counts);

                //Pre-allocate N-Grams
                pre_allocate_n_grams(counts);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            C2DMapTrie<MAX_LEVEL, WordIndexType>::~C2DMapTrie() {
                if (DO_SANITY_CHECKS) {
                    //Print the hash sizes statistics
                    for (int i = 0; i < MAX_LEVEL; i++) {
                        LOG_INFO3 << (i + 1) << "-Gram ctx hash [min,max]= [ " << m_hash_sizes[i].first << ", " << m_hash_sizes[i].second << " ]" << END_LOG;
                    }
                }

                //Deallocate One-Grams
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                }

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < (MAX_LEVEL - 2); idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&m_m_gram_map_ptrs[idx], &m_m_gram_alloc_ptrs[idx]);
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&m_n_gram_map_ptr, &m_n_gram_alloc_ptr);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DMapTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DMapTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DMapTrie, HashingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DMapTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DMapTrie, TOptCountWordIndex);
        }
    }
}