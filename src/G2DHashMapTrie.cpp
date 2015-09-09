/* 
 * File:   G2DHashMapTrie.cpp
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


#include "G2DHashMapTrie.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

using namespace uva::smt::tries::__G2DHashMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            const MemIncreaseStrategy * G2DHashMapTrie<N>::m_p_mem_strat = NULL;

            template<TModelLevel N>
            G2DHashMapTrie<N>::G2DHashMapTrie(AWordIndex * const _pWordIndex)
            : ATrie<N>(_pWordIndex), m_1_gram_data(NULL), m_N_gram_data(NULL) {
                //Initialize the array of number of gram ids per level
                memset(num_buckets, 0, N * sizeof (TShortId));

                //Clear the M-Gram bucket arrays
                memset(m_M_gram_data, 0, ATrie<N>::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket*));

                if (m_p_mem_strat == NULL) {
                    //Get the memory increase strategy
                    m_p_mem_strat = getMemIncreaseStrategy(__G2DHashMapTrie::MEM_INC_TYPE,
                            __G2DHashMapTrie::MIN_MEM_INC_NUM, __G2DHashMapTrie::MEM_INC_FACTOR);
                } else {
                    throw Exception("Re-initializing W2COrderedArrayTrie<N>::m_p_mem_strat");
                }

                LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
                LOG_INFO3 << "Using the " << m_p_mem_strat->getStrategyStr()
                        << "' memory allocation strategy." << END_LOG;
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::pre_allocate(const size_t counts[N]) {
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

            template<TModelLevel N>
            G2DHashMapTrie<N>::~G2DHashMapTrie() {
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
                //This is a static field, therefore we not only delete the value but re-set it to null
                if (m_p_mem_strat != NULL) {
                    delete m_p_mem_strat;
                    m_p_mem_strat = NULL;
                }
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::add_1_gram(const T_M_Gram &oGram) {
                //Register a new word, and the word id will be the one-gram id
                const TShortId oneGramId = ATrie<N>::get_word_index()->register_word(oGram.tokens[0]);
                //Store the probability data in the one gram data storage, under its id
                m_1_gram_data[oneGramId].prob = oGram.prob;
                m_1_gram_data[oneGramId].back_off = oGram.back_off;
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::add_m_gram(const T_M_Gram &mGram) {
                //Get the bucket index
                TShortId bucket_idx;
                get_bucket_id(mGram, bucket_idx);

                //Compute the M-gram level index
                const TModelLevel level_idx = (mGram.level - ATrie<N>::MGRAM_IDX_OFFSET);

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Back_Off_Entry & data = m_M_gram_data[level_idx][bucket_idx].get_new();

                //Create the M-gram id
                Compressed_M_Gram_Id::set_m_gram_id(mGram, this->get_word_index(), data.m_gram_id);

                //Set the probability and back-off data
                data.payload.prob = mGram.prob;
                data.payload.back_off = mGram.back_off;
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::add_n_gram(const T_M_Gram &nGram) {
                //Get the bucket index
                TShortId bucket_idx;
                get_bucket_id(nGram, bucket_idx);

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].get_new();

                //Create the M-gram id
                Compressed_M_Gram_Id::set_m_gram_id(nGram, this->get_word_index(), data.m_gram_id);

                //Set the probability data
                data.payload = nGram.prob;
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::post_m_grams(const TModelLevel level) {
                //Call the base class method first
                ATrie<N>::post_m_grams(level);

                //Compute the M-gram level index
                const TModelLevel level_idx = (level - ATrie<N>::MGRAM_IDX_OFFSET);

                //Sort the level's data
                post_M_N_Grams<TProbBackOffBucket>(m_M_gram_data[level_idx], level);
            }

            template<TModelLevel N>
            void G2DHashMapTrie<N>::post_n_grams() {
                //Call the base class method first
                ATrie<N>::post_n_grams();

                //Sort the level's data
                post_M_N_Grams<TProbBucket>(m_N_gram_data, N);
            };

            template<TModelLevel N>
            void G2DHashMapTrie<N>::query(const T_M_Gram & ngram, TQueryResult & result) {
                //ToDo: Implement
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class G2DHashMapTrie<M_GRAM_LEVEL_MAX>;
        }
    }
}