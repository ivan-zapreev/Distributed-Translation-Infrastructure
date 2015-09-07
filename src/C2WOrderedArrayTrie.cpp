/* 
 * File:   C2WOrderedArrayTrie.cpp
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
#include "C2WOrderedArrayTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            C2WOrderedArrayTrie<N>::C2WOrderedArrayTrie(AWordIndex * const p_word_index)
            : ALayeredTrie<N>(p_word_index,
            [&] (const TShortId wordId, TLongId & ctxId, const TModelLevel level) -> bool {

                return this->getContextId(wordId, ctxId, level); }),
            m_1_gram_data(NULL), m_N_gram_data(NULL) {

                //Memset the M grams reference and data arrays
                memset(m_M_gram_ctx_2_data, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TSubArrReference *));
                memset(m_M_gram_data, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TWordIdProbBackOffEntryPair *));

                //Initialize the array of counters
                memset(m_M_N_gram_num_ctx_ids, 0, ALayeredTrie<N>::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_N_gram_next_ctx_id, 0, ALayeredTrie<N>::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));

                LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
            }

            template<TModelLevel N>
            void C2WOrderedArrayTrie<N>::pre_allocate(const size_t counts[N]) {
                //01) Pre-allocate the word index super class call
                ALayeredTrie<N>::pre_allocate(counts);

                //02) Compute and store the M-gram level sizes in terms of the number of M-grams per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < ALayeredTrie<N>::NUM_M_N_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_N_gram_next_ctx_id[i] = ALayeredTrie<N>::FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_N_gram_num_ctx_ids[i] = counts[i + 1] + ALayeredTrie<N>::FIRST_VALID_CTX_ID;
                }

                //03) Pre-allocate the 1-Gram data
                //The size of this array is made two elements larger than the number
                //of 1-Grams is since we want to account for the word indexes that start
                //from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                const TShortId one_gram_arr_size = ATrie<N>::get_word_index()->get_words_count(counts[0]);
                m_1_gram_data = new TProbBackOffEntry[one_gram_arr_size];
                memset(m_1_gram_data, 0, one_gram_arr_size * sizeof (TProbBackOffEntry));

                //04) Insert the unknown word data into the allocated array
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                //05) Allocate data for the M-grams

                //First allocate the contexts to data mappings for the 2-grams (stored under index 0)
                //The number of contexts is the number of words in previous level 1 i.e. counts[0]
                //Yet we know that the word index begins with 2, due to UNDEFINED and UNKNOWN word ids
                //Therefore for the 2-gram level contexts array we add two more elements, just to simplify computations
                m_M_gram_ctx_2_data[0] = new TSubArrReference[one_gram_arr_size];
                memset(m_M_gram_ctx_2_data[0], 0, one_gram_arr_size * sizeof (TSubArrReference));

                //Now also allocate the data for the 2-Grams, the number of 2-grams is m_MN_gram_size[0] 
                m_M_gram_data[0] = new TWordIdProbBackOffEntryPair[m_M_N_gram_num_ctx_ids[0]];
                memset(m_M_gram_data[0], 0, m_M_N_gram_num_ctx_ids[0] * sizeof (TWordIdProbBackOffEntryPair));

                //Now the remaining elements can be added in a loop
                for (TModelLevel i = 1; i < ALayeredTrie<N>::NUM_M_GRAM_LEVELS; i++) {
                    //Here i is the index of the array, the corresponding M-gram
                    //level M = i + 2. The m_MN_gram_size[i-1] stores the number of elements
                    //on the previous level - the maximum number of possible contexts.
                    m_M_gram_ctx_2_data[i] = new TSubArrReference[m_M_N_gram_num_ctx_ids[i - 1]];
                    memset(m_M_gram_ctx_2_data[i], 0, m_M_N_gram_num_ctx_ids[i - 1] * sizeof (TSubArrReference));
                    //The m_MN_gram_size[i] stores the number of elements
                    //on the current level - the number of M-Grams.
                    m_M_gram_data[i] = new TWordIdProbBackOffEntryPair[m_M_N_gram_num_ctx_ids[i]];
                    memset(m_M_gram_data[i], 0, m_M_N_gram_num_ctx_ids[i] * sizeof (TWordIdProbBackOffEntryPair));
                }

                //06) Allocate the data for the N-Grams.
                m_N_gram_data = new TCtxIdProbEntry[m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR]];
                memset(m_N_gram_data, 0, m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR] * sizeof (TCtxIdProbEntry));
            }

            template<TModelLevel N>
            C2WOrderedArrayTrie<N>::~C2WOrderedArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < ALayeredTrie<N>::NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_M_gram_ctx_2_data[i];
                        delete[] m_M_gram_data[i];
                    }
                    delete[] m_N_gram_data;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class C2WOrderedArrayTrie<M_GRAM_LEVEL_MAX>;
        }
    }
}

