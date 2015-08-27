/* 
 * File:   W2COrderedArrayTrie.cpp
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
#include "W2COrderedArrayTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            W2COrderedArrayTrie<N>::W2COrderedArrayTrie(AWordIndex * const p_word_index)
            : ATrie<N>(p_word_index,
            [&] (const TShortId wordId, const TLongId ctxId, const TModelLevel level) -> TLongId {

                return this->getContextId(wordId, ctxId, level); }),
            m_num_word_ids(0), m_1_gram_data(NULL), m_N_gram_data(NULL) {

                //Memset the M/N grams reference and data arrays
                memset(m_M_N_gram_word_2_data, 0, NUM_M_N_GRAM_LEVELS * sizeof (TSubArrReference *));
                memset(m_M_gram_data, 0, NUM_M_GRAM_LEVELS * sizeof (TCtxIdProbBackOffEntryPair *));

                //Initialize the array of counters
                memset(m_M_N_gram_num_ctx_ids, 0, NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_N_gram_next_ctx_id, 0, NUM_M_N_GRAM_LEVELS * sizeof (TShortId));

                LOG_INFO3 << "Using the <" << __FILE__ << "> model. Collision "
                        << "detections are: " << (DO_SANITY_CHECKS ? "ON" : "OFF")
                        << " !" << END_LOG;
            }

            template<TModelLevel N>
            void W2COrderedArrayTrie<N>::preAllocate(const size_t counts[N]) {
                //Compute and store the M-gram level sizes in terms of the number of M-grams per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < NUM_M_N_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_N_gram_next_ctx_id[i] = FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_N_gram_num_ctx_ids[i] = counts[i + 1] + 1;
                }

                //01) Pre-allocate the word index
                ATrie<N>::getWordIndex()->reserve(counts[0]);

                //02) Pre-allocate the 1-Gram data
                m_num_word_ids = counts[0] + EXTRA_NUMBER_OF_WORD_IDs;
                m_1_gram_data = new TProbBackOffEntryPair[m_num_word_ids];
                memset(m_1_gram_data, 0, m_num_word_ids * sizeof (TProbBackOffEntryPair));

                //03) Insert the unknown word data into the allocated array
                TProbBackOffEntryPair & pbData = m_1_gram_data[UNKNOWN_WORD_ID];
                pbData.prob = MINIMAL_LOG_PROB_WEIGHT;
                pbData.back_off = UNDEFINED_LOG_PROB_WEIGHT;

                //04) Allocate data for the M-grams

                //First allocate the contexts to data mappings for the 2-grams (stored under index 0)
                //The number of contexts is the number of words in previous level 1 i.e. counts[0]
                //Yet we know that the word index begins with 2, due to UNDEFINED and UNKNOWN word ids
                //Therefore for the 2-gram level contexts array we add two more elements, just to simplify computations
                m_M_N_gram_word_2_data[0] = new TSubArrReference[m_num_word_ids];
                memset(m_M_N_gram_word_2_data[0], 0, m_num_word_ids * sizeof (TSubArrReference));

                //Now also allocate the data for the 2-Grams, the number of 2-grams is m_M_N_gram_num_ctx_ids[0] 
                m_M_gram_data[0] = new TCtxIdProbBackOffEntryPair[m_M_N_gram_num_ctx_ids[0]];
                memset(m_M_gram_data[0], 0, m_M_N_gram_num_ctx_ids[0] * sizeof (TCtxIdProbBackOffEntryPair));

                //Now the remaining elements can be added in a loop, iterate until NUM_M_GRAM_LEVELS because
                //for the last level - N-Gram the data is stored not in m_M_gram_data but in m_N_gram_data
                for (TModelLevel i = 1; i < NUM_M_GRAM_LEVELS; i++) {
                    //Here i is the index of the array, the corresponding M-gram
                    //level M = i + 2. The m_M_N_gram_num_ctx_ids[i-1] stores the number of elements
                    //on the previous level - the maximum number of possible contexts.
                    m_M_N_gram_word_2_data[i] = new TSubArrReference[m_M_N_gram_num_ctx_ids[i - 1]];
                    memset(m_M_N_gram_word_2_data[i], 0, m_M_N_gram_num_ctx_ids[i - 1] * sizeof (TSubArrReference));
                    //The m_MN_gram_size[i] stores the number of elements
                    //on the current level - the number of M-Grams.
                    m_M_gram_data[i] = new TCtxIdProbBackOffEntryPair[m_M_N_gram_num_ctx_ids[i]];
                    memset(m_M_gram_data[i], 0, m_M_N_gram_num_ctx_ids[i] * sizeof (TCtxIdProbBackOffEntryPair));
                }

                //05) Allocate the data for the N-Grams 
                
                //First allocate the N-Gram sub array mappings data in m_M_N_gram_word_2_data
                const TShortId n_gram_id = N - MGRAM_IDX_OFFSET;
                m_M_N_gram_word_2_data[n_gram_id] = new TSubArrReference[m_M_N_gram_num_ctx_ids[n_gram_id - 1]];
                memset(m_M_N_gram_word_2_data[n_gram_id], 0, m_M_N_gram_num_ctx_ids[n_gram_id - 1] * sizeof (TSubArrReference));

                //The number of elements is stored in counts[N - 1]. There is no need
                //to add extra elements here as the context index is not relevant.
                m_N_gram_data = new TCtxIdProbEntryPair[counts[N - 1]];
                memset(m_N_gram_data, 0, counts[N - 1] * sizeof (TCtxIdProbEntryPair));
            }

            template<TModelLevel N>
            W2COrderedArrayTrie<N>::~W2COrderedArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_M_N_gram_word_2_data[i];
                        delete[] m_M_gram_data[i];
                    }
                    delete[] m_M_N_gram_word_2_data[N - MGRAM_IDX_OFFSET];
                    delete[] m_N_gram_data;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class W2COrderedArrayTrie<MAX_NGRAM_LEVEL>;
        }
    }
}

