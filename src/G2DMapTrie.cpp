/* 
 * File:   G2DMapTrie.cpp
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
 * Created on September 8, 2015, 11:22 AM
 */

#include "G2DMapTrie.hpp"

#include <inttypes.h>   // std::uint32_t
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            G2DMapTrie<MAX_LEVEL, WordIndexType>::G2DMapTrie(WordIndexType & word_index)
            : GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::DO_BITMAP_HASH_CACHE>(word_index),
            m_1_gram_data(NULL), m_N_gram_data(NULL) {
                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL > M_GRAM_LEVEL_6), string("The maximum supported trie level is") + std::to_string(M_GRAM_LEVEL_6));
                ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");

                //Initialize the array of number of gram ids per level
                memset(m_num_buckets, 0, MAX_LEVEL * sizeof (TShortId));

                //Clear the M-Gram bucket arrays
                memset(m_M_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket*));

                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Back_Off_Entry)= " << sizeof (__G2DMapTrie::T_M_Gram_PB_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (__G2DMapTrie::T_M_Gram_Prob_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBackOffBucket)= " << sizeof (TProbBackOffBucket) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBucket)= " << sizeof (TProbBucket) << END_LOG;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the base-class
                BASE::pre_allocate(counts);

                //02) Pre-allocate the 1-Gram data
                m_num_buckets[0] = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new T_M_Gram_Payload[m_num_buckets[0]];
                memset(m_1_gram_data, 0, m_num_buckets[0] * sizeof (T_M_Gram_Payload));

                //03) Insert the unknown word data into the allocated array
                T_M_Gram_Payload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.m_back = ZERO_BACK_OFF_WEIGHT;

                //Compute the number of M-Gram level buckets and pre-allocate them
                for (TModelLevel idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    m_num_buckets[idx + 1] = max(counts[idx + 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                            __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                    m_M_gram_data[idx] = new TProbBackOffBucket[m_num_buckets[idx + 1]];
                }

                //Compute the number of N-Gram level buckets and pre-allocate them
                m_num_buckets[MAX_LEVEL - 1] = max(counts[MAX_LEVEL - 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                        __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                m_N_gram_data = new TProbBucket[m_num_buckets[MAX_LEVEL - 1]];
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            G2DMapTrie<MAX_LEVEL, WordIndexType>::~G2DMapTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    //De-allocate one grams
                    delete[] m_1_gram_data;
                    //De-allocate M-Grams
                    for (TModelLevel idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {

                        delete[] m_M_gram_data[idx];
                    }
                    //De-allocate N-Grams
                    delete[] m_N_gram_data;
                }
            };

            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, BasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, CountingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, HashingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptBasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptCountWordIndex);
        }
    }
}