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
            : GenericTrieBase<MAX_LEVEL, WordIndexType>(word_index),
            m_1_gram_data(NULL), m_N_gram_data(NULL) {
                //Initialize the array of number of gram ids per level
                memset(num_buckets, 0, MAX_LEVEL * sizeof (TShortId));

                //Clear the M-Gram bucket arrays
                memset(m_M_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket*));

                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Back_Off_Entry)= " << sizeof (T_M_Gram_PB_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (T_M_Gram_Prob_Entry) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBackOffBucket)= " << sizeof (TProbBackOffBucket) << END_LOG;
                LOG_DEBUG << "sizeof(TProbBucket)= " << sizeof (TProbBucket) << END_LOG;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //Call the base-class
                BASE::pre_allocate(counts);

                //02) Pre-allocate the 1-Gram data
                num_buckets[0] = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new T_M_Gram_Payload[num_buckets[0]];
                memset(m_1_gram_data, 0, num_buckets[0] * sizeof (T_M_Gram_Payload));

                //03) Insert the unknown word data into the allocated array
                T_M_Gram_Payload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back = ZERO_BACK_OFF_WEIGHT;

                //Compute the number of M-Gram level buckets and pre-allocate them
                for (TModelLevel idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    num_buckets[idx + 1] = max(counts[idx + 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                            __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                    m_M_gram_data[idx] = new TProbBackOffBucket[num_buckets[idx + 1]];
                }

                //Compute the number of N-Gram level buckets and pre-allocate them
                num_buckets[MAX_LEVEL - 1] = max(counts[MAX_LEVEL - 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                        __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                m_N_gram_data = new TProbBucket[num_buckets[MAX_LEVEL - 1]];
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

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_1_gram(const T_Model_M_Gram<WordIndexType> &gram) {
                //Get the word id of this unigram, so there is just one word in it and its the end one
                const TShortId word_id = gram.get_end_word_id();

                //Store the probability data in the one gram data storage, under its id
                m_1_gram_data[word_id].prob = gram.m_prob;
                m_1_gram_data[word_id].back = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel CURR_LEVEL>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //Get the bucket index
                LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << END_LOG;
                TShortId bucket_idx = get_bucket_id<CURR_LEVEL>(gram.get_hash());

                //Compute the M-gram level index
                constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                //Create a new M-Gram data entry
                T_M_Gram_PB_Entry & data = m_M_gram_data[LEVEL_IDX][bucket_idx].allocate();

                //Create the M-gram id from the word ids.
                gram.template create_m_gram_id<CURR_LEVEL>(data.id);

                //Set the probability and back-off data
                data.payload.prob = gram.m_prob;
                data.payload.back = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_n_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //Get the bucket index
                LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << END_LOG;
                TShortId bucket_idx = get_bucket_id<MAX_LEVEL>(gram.get_hash());

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();

                //Create the N-gram id from the word ids
                gram.template create_m_gram_id<MAX_LEVEL>(data.id);

                //Set the probability data
                data.payload = gram.m_prob;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<typename BUCKET_TYPE, TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX >
            bool G2DMapTrie<MAX_LEVEL, WordIndexType>::get_payload_from_gram_level(const T_Query_M_Gram<WordIndexType> & gram, const BUCKET_TYPE & ref,
                    typename BUCKET_TYPE::TElemType::TPayloadType & payload) const {
                //Compute the current level of the sub-m-gram
                constexpr TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;

                LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(CURR_LEVEL) << "-gram ["
                        << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;

                //1. Check that the bucket with the given index is not empty
                if (ref.has_data()) {
                    LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;
                    //2. Compute the query id
                    DECLARE_STACK_GRAM_ID(TM_Gram_Id, mgram_id, CURR_LEVEL);
                    T_Gram_Id_Data_Ptr mgram_id_ptr = &mgram_id[0];

                    //Create the M-gram id from the word ids.
                    gram.template create_m_gram_id<BEGIN_WORD_IDX, END_WORD_IDX>(mgram_id_ptr);

                    //3. Search for the query id in the bucket
                    //The data is available search for the word index in the array
                    typename BUCKET_TYPE::TIndexType found_idx;
                    if (search_gram<BUCKET_TYPE, CURR_LEVEL>(mgram_id_ptr, ref, found_idx)) {
                        payload = ref[found_idx].payload;
                        return true;
                    }
                }

                //Could not compute the probability for the given level, so backing off (recursive)!
                LOG_DEBUG << "Unable to find the " << SSTR(CURR_LEVEL) << "-gram, need to back off!" << END_LOG;
                return false;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
            GPR_Enum G2DMapTrie<MAX_LEVEL, WordIndexType>::get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const {
                //Compute the current level of the sub-m-gram
                const TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;

                LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(CURR_LEVEL) << "-gram ["
                        << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;

                //1. Check which level M-gram we need to get payload for
                if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    payload = m_1_gram_data[gram[END_WORD_IDX]];
                    LOG_DEBUG << "Getting the " << SSTR(CURR_LEVEL) << "-gram payload: " << (string) payload << END_LOG;
                    return GPR_Enum::PAYLOAD_GPR;
                } else {
                    //1.1. This is the case of the M-gram with M > 1
                    LOG_DEBUG << "The level " << SSTR(CURR_LEVEL) << "-gram max level is: " << MAX_LEVEL << END_LOG;

                    //1.1.2. Obtain the bucket id
                    LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(CURR_LEVEL) << "-gram ["
                            << BEGIN_WORD_IDX << "," << END_WORD_IDX << "] of: " << (string) gram << END_LOG;
                    const uint32_t bucket_idx = this->template get_bucket_id<CURR_LEVEL>(gram.template get_hash<BEGIN_WORD_IDX, END_WORD_IDX>());
                    LOG_DEBUG << "The " << SSTR(CURR_LEVEL) << "-gram hash bucket idx is: " << bucket_idx << END_LOG;

                    //1.1.4. Search for the probability on the given M-gram level
                    if (CURR_LEVEL == MAX_LEVEL) {
                        LOG_DEBUG << "Searching in " << SSTR(MAX_LEVEL) << "-grams" << END_LOG;
                        if (this->template get_payload_from_gram_level<TProbBucket, BEGIN_WORD_IDX, END_WORD_IDX>(gram, m_N_gram_data[bucket_idx], payload.prob)) {
                            LOG_DEBUG << "The " << SSTR(CURR_LEVEL) << "-gram is found, payload: " << (string) payload << END_LOG;
                            return GPR_Enum::PAYLOAD_GPR;
                        }
                    } else {
                        const TModelLevel M_GRAM_IDX = CURR_LEVEL - BASE::MGRAM_IDX_OFFSET;
                        LOG_DEBUG << "Searching in " << SSTR(CURR_LEVEL) << "-grams, array index: " << M_GRAM_IDX << END_LOG;
                        if (this->template get_payload_from_gram_level<TProbBackOffBucket, BEGIN_WORD_IDX, END_WORD_IDX>(gram, m_M_gram_data[M_GRAM_IDX][bucket_idx], payload)) {
                            LOG_DEBUG << "The " << SSTR(CURR_LEVEL) << "-gram is found, payload: " << (string) payload << END_LOG;
                            return GPR_Enum::PAYLOAD_GPR;
                        }
                    }
                    LOG_DEBUG << "The " << SSTR(CURR_LEVEL) << "-gram payload is not found!" << END_LOG;

                    //If we are asked to retrieve the back-off payload then try to do it now
                    if (DO_BACK_OFF) {
                        LOG_DEBUG << "Asked to retrieve the back-off payload!" << END_LOG;
                        constexpr TModelLevel BO_CURR_LEVEL = CURR_LEVEL - 1;
                        constexpr TModelLevel BO_END_WORD_IDX = ((BEGIN_WORD_IDX < END_WORD_IDX) ? (END_WORD_IDX - 1) : END_WORD_IDX);
                        if (get_payload<BEGIN_WORD_IDX, BO_END_WORD_IDX, false > (gram, bo_payload, bo_payload) == GPR_Enum::PAYLOAD_GPR) {
                            LOG_DEBUG << "The " << SSTR(BO_CURR_LEVEL) << "-gram is found, back-off payload: " << (string) bo_payload << END_LOG;
                            return GPR_Enum::BACK_OFF_GPR;
                        }
                        LOG_DEBUG << "The " << SSTR(BO_CURR_LEVEL) << "-gram payload is also not found!" << END_LOG;
                    }

                    return GPR_Enum::FAILED_GPR;
                }
            }

            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, BasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, CountingWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptBasicWordIndex);
            INSTANTIATE_TRIE_TEMPLATE_TYPE(G2DMapTrie, M_GRAM_LEVEL_MAX, TOptCountWordIndex);
        }
    }
}