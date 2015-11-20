/* 
 * File:   W2CArrayTrie.cpp
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
 * Created on August 27, 2015, 08:33 PM
 */
#include "W2CArrayTrie.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;

using namespace __W2CArrayTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            W2CArrayTrie<MAX_LEVEL, WordIndexType>::W2CArrayTrie(WordIndexType & word_index)
            : LayeredTrieBase<W2CArrayTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __W2CArrayTrie::DO_BITMAP_HASH_CACHE>(word_index),
            m_num_word_ids(0), m_1_gram_data(NULL), m_n_gram_word_2_data(NULL) {
                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");

                //Memset the M/N grams reference and data arrays
                memset(m_m_gram_word_2_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (T_M_GramWordEntry *));
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void W2CArrayTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //01) Pre-allocate the word index super class call
                BASE::pre_allocate(counts);

                //02) Pre-allocate the 1-Gram data
                m_num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new T_M_Gram_Payload[m_num_word_ids];
                memset(m_1_gram_data, 0, m_num_word_ids * sizeof (T_M_Gram_Payload));

                //03) Insert the unknown word data into the allocated array
                T_M_Gram_Payload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.m_prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.m_back = ZERO_BACK_OFF_WEIGHT;

                //04) Allocate data for the M-grams

                for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                    preAllocateWordsData<T_M_GramWordEntry>(m_m_gram_word_2_data[i], counts[i + 1], counts[0]);
                }

                //05) Allocate the data for the N-Grams 
                preAllocateWordsData<T_N_GramWordEntry>(m_n_gram_word_2_data, counts[MAX_LEVEL - 1], counts[0]);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            W2CArrayTrie<MAX_LEVEL, WordIndexType>::~W2CArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_m_gram_word_2_data[i];
                    }
                    delete[] m_n_gram_word_2_data;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, HashingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, TOptCountWordIndex);
        }
    }
}

