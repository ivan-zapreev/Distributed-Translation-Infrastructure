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
                m_1_gram_data = new TMGramPayload[num_buckets[0]];
                memset(m_1_gram_data, 0, num_buckets[0] * sizeof (TMGramPayload));

                //03) Insert the unknown word data into the allocated array
                TMGramPayload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;

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
                //Get the word id of this one gram
                const TShortId word_id = gram.get_end_word_id();

                //Store the probability data in the one gram data storage, under its id
                m_1_gram_data[word_id].prob = gram.m_prob;
                m_1_gram_data[word_id].back_off = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel CURR_LEVEL>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //Get the bucket index

                TShortId bucket_idx = get_bucket_id<CURR_LEVEL>(gram);

                //Compute the M-gram level index
                constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                //Create a new M-Gram data entry
                T_M_Gram_PB_Entry & data = m_M_gram_data[LEVEL_IDX][bucket_idx].allocate();

                //Create the M-gram id from the word ids.
                gram.template create_m_gram_id<CURR_LEVEL>(data.id);

                //Set the probability and back-off data
                data.payload.prob = gram.m_prob;
                data.payload.back_off = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_n_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //Get the bucket index

                TShortId bucket_idx = get_bucket_id<MAX_LEVEL>(gram);

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();

                //Create the N-gram id from the word ids
                gram.template create_m_gram_id<MAX_LEVEL>(data.id);

                //Set the probability data
                data.payload = gram.m_prob;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<typename BUCKET_TYPE, bool IS_BACK_OFF, TModelLevel CURR_LEVEL >
            bool G2DMapTrie<MAX_LEVEL, WordIndexType>::get_payload_from_gram_level(const T_M_Gram<WordIndexType> & gram, const BUCKET_TYPE & ref,
                    const typename BUCKET_TYPE::TElemType::TPayloadType * & payload_ptr) const {
                //1. Check that the bucket with the given index is not empty
                if (ref.has_data()) {
                    LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;
                    //2. Compute the query id
                    DECLARE_STACK_GRAM_ID(TM_Gram_Id, mgram_id, CURR_LEVEL);
                    T_Gram_Id_Data_Ptr mgram_id_ptr = &mgram_id[0];

                    //Create the M-gram id from the word ids.
                    gram.template create_m_gram_id<CURR_LEVEL, IS_BACK_OFF>(mgram_id_ptr);

                    //3. Search for the query id in the bucket
                    //The data is available search for the word index in the array
                    typename BUCKET_TYPE::TIndexType found_idx;
                    if (search_gram<BUCKET_TYPE, CURR_LEVEL>(mgram_id_ptr, ref, found_idx)) {
                        payload_ptr = &ref[found_idx].payload;
                        return true;
                    }
                }

                //Could not compute the probability for the given level, so backing off (recursive)!
                LOG_DEBUG << "Unable to find the " << SSTR(CURR_LEVEL) << "-Gram, need to back off!" << END_LOG;
                return false;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel CURR_LEVEL>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::get_prob_weight(const T_M_Gram<WordIndexType> & gram, SQueryResult<MAX_LEVEL> & result) const {
                LOG_DEBUG << "Computing probability for a " << CURR_LEVEL << "-gram " << END_LOG;

                //1. Check which level M-gram we need to get probability for
                if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1
                    LOG_DEBUG << "The level " << CURR_LEVEL << "-gram max level " << MAX_LEVEL << END_LOG;

                    //1.1.2. Obtain the bucket id
                    const uint32_t bucket_idx = get_bucket_id<false, CURR_LEVEL>(gram);
                    LOG_DEBUG << "The " << CURR_LEVEL << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4. Search for the probability on the given M-gram level
                    if (CURR_LEVEL == MAX_LEVEL) {
                        LOG_DEBUG << "Searching in N grams" << END_LOG;

                        //1.1.4.1 This is an N-gram case
                        const typename TProbBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBucket, false, CURR_LEVEL>(gram, m_N_gram_data[bucket_idx], payload_ptr)) {
                            //1.1.4.1.1 The probability is nicely found
                            result.m_total_prob = *payload_ptr;
                            LOG_DEBUG << "The N-gram is found, prob: " << result.m_total_prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The N-gram is not found!" << END_LOG;
                            //1.1.4.1.2 Could not compute the probability for the given level, so backing off (recursive)!
                        }
                    } else {
                        const TModelLevel mgram_indx = CURR_LEVEL - BASE::MGRAM_IDX_OFFSET;
                        LOG_DEBUG << "Searching in " << CURR_LEVEL << "-grams, array index: " << mgram_indx << END_LOG;

                        //1.1.4.2 This is an M-gram case (1 < M < N))
                        const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBackOffBucket, false, CURR_LEVEL>(gram, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                            //1.1.4.2.1 The probability is nicely found
                            result.m_total_prob = payload_ptr->prob;
                            LOG_DEBUG << "The " << CURR_LEVEL << "-gram is found, prob: " << result.m_total_prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The " << CURR_LEVEL << "-gram is not found!" << END_LOG;
                            //1.1.4.2.2 Could not compute the probability for the given level, so backing off (recursive)!
                        }
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    result.m_total_prob = m_1_gram_data[gram.get_end_word_id()].prob;
                    LOG_DEBUG << "Getting the " << CURR_LEVEL << "-gram prob: " << result.m_total_prob << END_LOG;
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel CURR_LEVEL>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_back_off_weight(const T_M_Gram<WordIndexType> & gram, SQueryResult<MAX_LEVEL> & result) const {
                LOG_DEBUG << "Computing back-off for a " << CURR_LEVEL << "-gram " << END_LOG;

                //Perform a sanity check if needed
                if (DO_SANITY_CHECKS && (CURR_LEVEL < M_GRAM_LEVEL_1)) {
                    throw Exception("Trying to obtain a back-off weight for level zero!");
                }

                //1. Check which level M-gram we need to get probability for
                if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1 and clearly M < N
                    LOG_DEBUG << "The level " << CURR_LEVEL << "-gram max level " << MAX_LEVEL << END_LOG;

                    //1.1.2. Obtain the bucket id
                    const uint32_t bucket_idx = get_bucket_id<true, CURR_LEVEL>(gram);
                    LOG_DEBUG << "The " << CURR_LEVEL << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4 This is an M-gram case (1 < M < N))
                    const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                    const TModelLevel mgram_indx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                    if (get_payload_from_gram_level<TProbBackOffBucket, true, CURR_LEVEL>(gram, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                        //1.1.4.1 The probability is nicely found
                        result.m_total_prob += payload_ptr->back_off;
                        LOG_DEBUG << "The " << CURR_LEVEL << "-gram is found, back_off: " << payload_ptr->back_off << END_LOG;
                    } else {
                        //The query context id could be determined, but 
                        //the data was not found in the trie.
                        LOG_DEBUG << "Unable to find back-off data for "
                                << CURR_LEVEL << "-Gram query!" << END_LOG;
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    TLogProbBackOff back_off = m_1_gram_data[gram.get_back_off_end_word_id()].back_off;
                    result.m_total_prob += back_off;
                    LOG_DEBUG << "Getting the " << CURR_LEVEL << "-gram back_off: " << back_off << END_LOG;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TYPE, TYPE_EXT) \
            template class G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_1>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_2>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_3>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_4>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_5>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_6>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_7>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_1>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_2>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_3>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_4>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_5>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_6>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_7>(const T_M_Gram<TYPE > & gram, SQueryResult<M_GRAM_LEVEL_MAX> & result) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_2>(const T_Model_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_3>(const T_Model_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_4>(const T_Model_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_5>(const T_Model_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_6>(const T_Model_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_7>(const T_Model_M_Gram<TYPE > & gram);

            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(BasicWordIndex, Basic);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(CountingWordIndex, Count);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TOptBasicWordIndex, OptBasic);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TOptCountWordIndex, OptCount);
        }
    }
}