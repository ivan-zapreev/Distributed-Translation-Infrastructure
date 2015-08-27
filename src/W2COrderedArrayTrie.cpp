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

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            using namespace __W2COrderedArrayTrie;

            template<TModelLevel N>
            W2COrderedArrayTrie<N>::W2COrderedArrayTrie(AWordIndex * const p_word_index)
            : ATrie<N>(p_word_index,
            [&] (const TShortId wordId, const TLongId ctxId, const TModelLevel level) -> TLongId {

                return this->getContextId(wordId, ctxId, level); }),
            m_num_word_ids(0), m_1_gram_data(NULL), m_N_gram_word_2_data(NULL), m_get_capacity_inc_func(NULL) {

                //Memset the M/N grams reference and data arrays
                memset(m_M_gram_word_2_data, 0, NUM_M_GRAM_LEVELS * sizeof (T_M_GramWordEntry *));

                //ToDo: Optimize this and make mapping from enumeration to corresponding string.
                switch (MEM_INC_TYPE) {
                    case MemIncTypesEnum::CONSTANT:
                        LOG_INFO3 << "The capacity increase strategy in " << __FILE__ << " is MemIncTypesEnum::CONSTANT" << END_LOG;
                        m_get_capacity_inc_func = [] (const float fcap) -> float {
                            //Return zero as then the minimum constant increase will be used!
                            return 0;
                        };
                        break;
                    case MemIncTypesEnum::LINEAR:
                        LOG_INFO3 << "The capacity increase strategy in " << __FILE__ << " is MemIncTypesEnum::LINEAR" << END_LOG;
                        m_get_capacity_inc_func = [] (const float fcap) -> float {
                            return MAX_MEM_INC_PRCT * fcap;
                        };
                        break;
                    case MemIncTypesEnum::LOG_2:
                        LOG_INFO3 << "The capacity increase strategy in " << __FILE__ << " is MemIncTypesEnum::LOG_2" << END_LOG;
                        m_get_capacity_inc_func = [] (const float fcap) -> float {
                            return MAX_MEM_INC_PRCT * fcap / log(fcap);
                        };
                        break;
                    case MemIncTypesEnum::LOG_10:
                        LOG_INFO3 << "The capacity increase strategy in " << __FILE__ << " is MemIncTypesEnum::LOG_10" << END_LOG;
                        m_get_capacity_inc_func = [] (const float fcap) -> float {
                            //Get the float capacity value, make it minimum of one element to avoid problems
                            return MAX_MEM_INC_PRCT * fcap / log10(fcap);
                        };
                        break;
                    default:
                        stringstream msg;
                        msg << "Unrecognized memory allocation strategy: " << __W2COrderedArrayTrie::MEM_INC_TYPE;
                        throw Exception(msg.str());
                }

                LOG_INFO3 << "Using the <" << __FILE__ << "> model. Collision "
                        << "detections are: " << (DO_SANITY_CHECKS ? "ON" : "OFF")
                        << " !" << END_LOG;
            }

            template<TModelLevel N>
            void W2COrderedArrayTrie<N>::preAllocate(const size_t counts[N]) {
                //01) Pre-allocate the word index
                ATrie<N>::getWordIndex()->reserve(counts[0]);

                //02) Pre-allocate the 1-Gram data
                m_num_word_ids = counts[0] + EXTRA_NUMBER_OF_WORD_IDs;
                m_1_gram_data = new TProbBackOffEntry[m_num_word_ids];
                memset(m_1_gram_data, 0, m_num_word_ids * sizeof (TProbBackOffEntry));

                //03) Insert the unknown word data into the allocated array
                TProbBackOffEntry & pbData = m_1_gram_data[UNKNOWN_WORD_ID];
                pbData.prob = MINIMAL_LOG_PROB_WEIGHT;
                pbData.back_off = UNDEFINED_LOG_PROB_WEIGHT;

                //04) Allocate data for the M-grams

                for (TModelLevel i = 0; i < NUM_M_GRAM_LEVELS; i++) {
                    preAllocateWordsData<T_M_GramWordEntry>(m_M_gram_word_2_data[i], counts[i + 1], counts[0]);
                }

                //05) Allocate the data for the N-Grams 
                preAllocateWordsData<T_N_GramWordEntry>(m_N_gram_word_2_data, counts[N - 1], counts[0]);
            }

            template<TModelLevel N>
            W2COrderedArrayTrie<N>::~W2COrderedArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < NUM_M_GRAM_LEVELS; i++) {
                        deAllocateWordsData(m_M_gram_word_2_data[i]);
                    }
                    deAllocateWordsData(m_N_gram_word_2_data);
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class W2COrderedArrayTrie<MAX_NGRAM_LEVEL>;
        }
    }
}

