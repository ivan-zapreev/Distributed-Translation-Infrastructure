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
                m_1_gram_data = new TProbBackOffEntry[num_buckets[0]];
                memset(m_1_gram_data, 0, num_buckets[0] * sizeof (TProbBackOffEntry));

                //03) Insert the unknown word data into the allocated array
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
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
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_1_gram(const T_M_Gram<WordIndexType> &gram) {
                //Get the word id of this one gram
                const TShortId word_id = gram.get_end_word_id();

                //Store the probability data in the one gram data storage, under its id
                m_1_gram_data[word_id].prob = gram.m_prob;
                m_1_gram_data[word_id].back_off = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel level>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_m_gram(const T_M_Gram<WordIndexType> &gram) {
                //Get the bucket index

                TShortId bucket_idx;
                get_bucket_id(gram, bucket_idx);

                //Compute the M-gram level index
                const TModelLevel level_idx = (gram.m_used_level - BASE::MGRAM_IDX_OFFSET);

                //Create a new M-Gram data entry
                T_M_Gram_PB_Entry & data = m_M_gram_data[level_idx][bucket_idx].allocate();

                //Create the M-gram id from the word ids.
                constexpr TModelLevel begin_idx = (T_M_Gram<WordIndexType>::MAX_LEVEL - level);
                Byte_M_Gram_Id::create_m_gram_id<begin_idx, level>(gram.m_word_ids, data.id);
                LOG_DEBUG << "Allocated M-gram id " << (void*) data.id << " for " << (string) gram << END_LOG;

                //Set the probability and back-off data
                data.payload.prob = gram.m_prob;
                data.payload.back_off = gram.m_back_off;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_n_gram(const T_M_Gram<WordIndexType> &gram) {
                //Get the bucket index

                TShortId bucket_idx;
                get_bucket_id(gram, bucket_idx);

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();

                //Create the N-gram id from the word ids
                constexpr TModelLevel begin_idx = (T_M_Gram<WordIndexType>::MAX_LEVEL - MAX_LEVEL);
                Byte_M_Gram_Id::create_m_gram_id<begin_idx, MAX_LEVEL>(gram.m_word_ids, data.id);
                LOG_DEBUG << "Allocated M-gram id " << (void*) data.id << " for " << (string) gram << END_LOG;

                //Set the probability data
                data.payload = gram.m_prob;
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::post_m_grams(const TModelLevel level) {
                //Call the base class method first
                if (BASE::is_post_grams(level)) {
                    BASE::post_m_grams(level);
                }

                //Compute the M-gram level index
                const TModelLevel level_idx = (level - BASE::MGRAM_IDX_OFFSET);

                //Sort the level's data
                post_M_N_Grams<TProbBackOffBucket>(m_M_gram_data[level_idx], level);
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::post_n_grams() {
                //Call the base class method first
                if (BASE::is_post_grams(MAX_LEVEL)) {
                    BASE::post_n_grams();
                }


                //Sort the level's data
                post_M_N_Grams<TProbBucket>(m_N_gram_data, MAX_LEVEL);
            };

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<typename BUCKET_TYPE, bool back_off, TModelLevel curr_level >
            bool G2DMapTrie<MAX_LEVEL, WordIndexType>::get_payload_from_gram_level(const MGramQuery<WordIndexType> & query, const BUCKET_TYPE & ref,
                    const typename BUCKET_TYPE::TElemType::TPayloadType * & payload_ptr) const {
                //Compute the begin index in the tokens and word ids arrays.
                constexpr TModelLevel begin_idx = (back_off ? ((T_M_Gram<WordIndexType>::MAX_LEVEL - 1) - curr_level) : (T_M_Gram<WordIndexType>::MAX_LEVEL - curr_level));
                LOG_DEBUG << "Retrieving payload for " << curr_level << "-gram word id indexes: ["
                        << begin_idx << "," << (back_off ? (MAX_LEVEL - 2) : (MAX_LEVEL - 1)) << "]" << END_LOG;

                //1. Check that the bucket with the given index is not empty
                if (ref.has_data()) {
                    LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;
                    //2. Compute the query id
                    DECLARE_STACK_GRAM_ID(mgram_id, curr_level);
                    T_Gram_Id_Storage_Ptr mgram_id_ptr = &mgram_id[0];

                    Byte_M_Gram_Id::create_m_gram_id<begin_idx, curr_level>(query.m_gram.m_word_ids, mgram_id_ptr);

                    //3. Search for the query id in the bucket
                    //The data is available search for the word index in the array
                    typename BUCKET_TYPE::TIndexType found_idx;
                    if (search_gram<BUCKET_TYPE, curr_level>(mgram_id_ptr, ref, found_idx)) {
                        payload_ptr = &ref[found_idx].payload;
                        return true;
                    }
                }

                //Could not compute the probability for the given level, so backing off (recursive)!
                LOG_DEBUG << "Unable to find the " << SSTR(curr_level) << "-Gram, need to back off!" << END_LOG;
                return false;
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel curr_level>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::get_prob_weight(MGramQuery<WordIndexType> & query) const {
                LOG_DEBUG << "Computing probability for a " << curr_level << "-gram " << END_LOG;

                //1. Check which level M-gram we need to get probability for
                if (curr_level > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1
                    LOG_DEBUG << "The level " << curr_level << "-gram max level " << MAX_LEVEL << END_LOG;

                    //1.1.2. Compute the m-gram hash
                    uint64_t gram_hash = query.m_gram.template level_hash<false, curr_level>();

                    //1.1.3. Search for the bucket
                    const uint32_t bucket_idx = get_bucket_id<curr_level>(gram_hash);
                    LOG_DEBUG << "The " << curr_level << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4. Search for the probability on the given M-gram level
                    if (curr_level == MAX_LEVEL) {
                        LOG_DEBUG << "Searching in N grams" << END_LOG;

                        //1.1.4.1 This is an N-gram case
                        const typename TProbBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBucket, false, curr_level>(query, m_N_gram_data[bucket_idx], payload_ptr)) {
                            //1.1.4.1.1 The probability is nicely found
                            query.m_result.m_prob = *payload_ptr;
                            LOG_DEBUG << "The N-gram is found, prob: " << query.m_result.m_prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The N-gram is not found!" << END_LOG;
                            //1.1.4.1.2 Could not compute the probability for the given level, so backing off (recursive)!
                        }
                    } else {
                        const TModelLevel mgram_indx = curr_level - BASE::MGRAM_IDX_OFFSET;
                        LOG_DEBUG << "Searching in " << curr_level << "-grams, array index: " << mgram_indx << END_LOG;

                        //1.1.4.2 This is an M-gram case (1 < M < N))
                        const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBackOffBucket, false, curr_level>(query, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                            //1.1.4.2.1 The probability is nicely found
                            query.m_result.m_prob = payload_ptr->prob;
                            LOG_DEBUG << "The " << curr_level << "-gram is found, prob: " << query.m_result.m_prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The " << curr_level << "-gram is not found!" << END_LOG;
                            //1.1.4.2.2 Could not compute the probability for the given level, so backing off (recursive)!
                        }
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    query.m_result.m_prob = m_1_gram_data[query.get_end_word_id()].prob;
                    LOG_DEBUG << "Getting the " << curr_level << "-gram prob: " << query.m_result.m_prob << END_LOG;
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            template<TModelLevel curr_level>
            void G2DMapTrie<MAX_LEVEL, WordIndexType>::add_back_off_weight(MGramQuery<WordIndexType> & query) const {
                LOG_DEBUG << "Computing back-off for a " << curr_level << "-gram " << END_LOG;

                //Perform a sanity check if needed
                if (DO_SANITY_CHECKS && (curr_level < M_GRAM_LEVEL_1)) {
                    throw Exception("Trying to obtain a back-off weight for level zero!");
                }

                //1. Check which level M-gram we need to get probability for
                if (curr_level > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1 and clearly M < N
                    LOG_DEBUG << "The level " << curr_level << "-gram max level " << MAX_LEVEL << END_LOG;

                    //1.1.2. Compute the hash value for the back off M-gram
                    uint64_t gram_hash = query.m_gram.template level_hash<true, curr_level>();

                    //1.1.3. Search for the bucket
                    const uint32_t bucket_idx = get_bucket_id<curr_level>(gram_hash);
                    LOG_DEBUG << "The " << curr_level << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4 This is an M-gram case (1 < M < N))
                    const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                    const TModelLevel mgram_indx = (curr_level - BASE::MGRAM_IDX_OFFSET);
                    if (get_payload_from_gram_level<TProbBackOffBucket, true, curr_level>(query, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                        //1.1.4.1 The probability is nicely found
                        query.m_result.m_prob += payload_ptr->back_off;
                        LOG_DEBUG << "The " << curr_level << "-gram is found, back_off: " << payload_ptr->back_off << END_LOG;
                    } else {
                        //The query context id could be determined, but 
                        //the data was not found in the trie.
                        LOG_DEBUG << "Unable to find back-off data for "
                                << curr_level << "-Gram query!" << END_LOG;
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    TLogProbBackOff back_off = m_1_gram_data[query.get_back_off_end_word_id()].back_off;
                    query.m_result.m_prob += back_off;
                    LOG_DEBUG << "Getting the " << curr_level << "-gram back_off: " << back_off << END_LOG;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TYPE, TYPE_EXT) \
            template class G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_1>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_2>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_3>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_4>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_5>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_6>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::get_prob_weight<M_GRAM_LEVEL_7>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_1>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_2>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_3>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_4>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_5>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_6>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_back_off_weight<M_GRAM_LEVEL_7>(TMGramQuery##TYPE_EXT & query) const; \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_2>(const T_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_3>(const T_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_4>(const T_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_5>(const T_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_6>(const T_M_Gram<TYPE > & gram); \
            template void G2DMapTrie<M_GRAM_LEVEL_MAX, TYPE >::add_m_gram<M_GRAM_LEVEL_7>(const T_M_Gram<TYPE > & gram);

            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(BasicWordIndex, Basic);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(CountingWordIndex, Count);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TOptBasicWordIndex, OptBasic);
            INSTANTIATE_G2D_TRIE_TEMPLATES_TYPE(TOptCountWordIndex, OptCount);
        }
    }
}