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

using namespace uva::smt::tries::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            G2DMapTrie<N>::G2DMapTrie(AWordIndex * const _pWordIndex)
            : ATrie<N>(_pWordIndex), m_tmp_gram_id(), m_1_gram_data(NULL), m_N_gram_data(NULL) {
                //Initialize the array of number of gram ids per level
                memset(num_buckets, 0, N * sizeof (TShortId));

                //Clear the M-Gram bucket arrays
                memset(m_M_gram_data, 0, ATrie<N>::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffBucket*));

                //Allocate the M-gram id memory for queries
                Comp_M_Gram_Id::allocate_m_gram_id(N, m_tmp_gram_id);

                LOG_INFO3 << "Using the <" << __FILE__ << "> model with the #buckets divider: "
                        << SSTR(__G2DMapTrie::WORDS_PER_BUCKET_FACTOR) << END_LOG;
                LOG_INFO3 << "Using the " << T_M_Gram_PB_Entry::m_mem_strat.getStrategyStr()
                        << " memory allocation strategy." << END_LOG;

                LOG_DEBUG3 << "sizeof(T_M_Gram_Prob_Back_Off_Entry)= " << sizeof (T_M_Gram_PB_Entry) << END_LOG;
                LOG_DEBUG3 << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (T_M_Gram_Prob_Entry) << END_LOG;
                LOG_DEBUG3 << "sizeof(TProbBackOffBucket)= " << sizeof (TProbBackOffBucket) << END_LOG;
                LOG_DEBUG3 << "sizeof(TProbBucket)= " << sizeof (TProbBucket) << END_LOG;
            };

            template<TModelLevel N>
            void G2DMapTrie<N>::pre_allocate(const size_t counts[N]) {
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
                    num_buckets[idx + 1] = max(counts[idx + 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                            __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                    m_M_gram_data[idx] = new TProbBackOffBucket[num_buckets[idx + 1]];
                }

                //Compute the number of N-Gram level buckets and pre-allocate them
                num_buckets[N - 1] = max(counts[N - 1] / __G2DMapTrie::WORDS_PER_BUCKET_FACTOR,
                        __G2DMapTrie::WORDS_PER_BUCKET_FACTOR);
                m_N_gram_data = new TProbBucket[num_buckets[N - 1]];
            };

            template<TModelLevel N>
            G2DMapTrie<N>::~G2DMapTrie() {
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
                //Destroy the query id if any
                Comp_M_Gram_Id::destroy(m_tmp_gram_id);
            };

            template<TModelLevel N>
            void G2DMapTrie<N>::add_1_gram(const T_M_Gram &oGram) {
                //Register a new word, and the word id will be the one-gram id

                const TShortId oneGramId = ATrie<N>::get_word_index()->register_word(oGram.tokens[0]);
                //Store the probability data in the one gram data storage, under its id
                m_1_gram_data[oneGramId].prob = oGram.prob;
                m_1_gram_data[oneGramId].back_off = oGram.back_off;
            };

            template<TModelLevel N>
            void G2DMapTrie<N>::add_m_gram(const T_M_Gram &mGram) {
                //Get the bucket index

                TShortId bucket_idx;
                get_bucket_id(mGram, bucket_idx);

                //Compute the M-gram level index
                const TModelLevel level_idx = (mGram.level - ATrie<N>::MGRAM_IDX_OFFSET);

                //Create a new M-Gram data entry
                T_M_Gram_PB_Entry & data = m_M_gram_data[level_idx][bucket_idx].allocate();

                //Get the N-gram word ids
                ATrie<N>::store_m_gram_word_ids(mGram);

                //Create the M-gram id from the word ids
                Comp_M_Gram_Id::create_m_gram_id(ATrie<N>::m_tmp_word_ids, N - mGram.level, mGram.level, data.id);
                LOG_DEBUG2 << "Allocated M-gram id " << (void*) data.id << " for " << tokensToString(mGram) << END_LOG;

                //Set the probability and back-off data
                data.payload.prob = mGram.prob;
                data.payload.back_off = mGram.back_off;
            };

            template<TModelLevel N>
            void G2DMapTrie<N>::add_n_gram(const T_M_Gram &nGram) {
                //Get the bucket index

                TShortId bucket_idx;
                get_bucket_id(nGram, bucket_idx);

                //Create a new M-Gram data entry
                T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();

                //Get the N-gram word ids
                ATrie<N>::store_m_gram_word_ids(nGram);

                //Create the N-gram id from the word ids
                Comp_M_Gram_Id::create_m_gram_id(ATrie<N>::m_tmp_word_ids, 0, N, data.id);
                LOG_DEBUG3 << "Allocated M-gram id " << (void*) data.id << " for " << tokensToString(nGram) << END_LOG;

                //Set the probability data
                data.payload = nGram.prob;
            };

            template<TModelLevel N>
            void G2DMapTrie<N>::post_m_grams(const TModelLevel level) {
                //Call the base class method first

                ATrie<N>::post_m_grams(level);

                //Compute the M-gram level index
                const TModelLevel level_idx = (level - ATrie<N>::MGRAM_IDX_OFFSET);

                //Sort the level's data
                post_M_N_Grams<TProbBackOffBucket>(m_M_gram_data[level_idx], level);
            }

            template<TModelLevel N>
            void G2DMapTrie<N>::post_n_grams() {
                //Call the base class method first

                ATrie<N>::post_n_grams();

                //Sort the level's data
                post_M_N_Grams<TProbBucket>(m_N_gram_data, N);
            };

            template<TModelLevel N>
            template<typename BUCKET_TYPE, bool back_off >
            bool G2DMapTrie<N>::get_payload_from_gram_level(const TModelLevel level, const BUCKET_TYPE & ref,
                    const typename BUCKET_TYPE::TElemType::TPayloadType * & payload_ptr) {
                //Compute the begin index in the tokens and word ids arrays
                const TModelLevel elem_begin_idx = (back_off ? ((N - 1) - level) : (N - level));
                LOG_DEBUG2 << "Retrieving payload for " << level << "-gram word id indexes: ["
                        << elem_begin_idx << "," << (back_off ? (N - 2) : (N - 1)) << "]" << END_LOG;

                //1. Check that the bucket with the given index is not empty
                if (ref.has_data()) {
                    LOG_DEBUG2 << "The bucket contains " << ref.size() << " elements!" << END_LOG;
                    //2. Compute the query id
                    Comp_M_Gram_Id::create_m_gram_id(ATrie<N>::m_tmp_word_ids, elem_begin_idx, level, m_tmp_gram_id);

                    //3. Search for the query id in the bucket
                    //The data is available search for the word index in the array
                    typename BUCKET_TYPE::TIndexType found_idx;
                    bool is_found = false;
                    switch (level) {
                        case M_GRAM_LEVEL_2:
                            is_found = search_gram<BUCKET_TYPE, M_GRAM_LEVEL_2>(ref, found_idx);
                            break;
                        case M_GRAM_LEVEL_3:
                            is_found = search_gram<BUCKET_TYPE, M_GRAM_LEVEL_3>(ref, found_idx);
                            break;
                        case M_GRAM_LEVEL_4:
                            is_found = search_gram<BUCKET_TYPE, M_GRAM_LEVEL_4>(ref, found_idx);
                            break;
                        case M_GRAM_LEVEL_5:
                            is_found = search_gram<BUCKET_TYPE, M_GRAM_LEVEL_5>(ref, found_idx);
                            break;
                        default:
                            stringstream msg;
                            msg << "Unsupported M-gram level: " << SSTR(level)
                                    << " must be within [ " << SSTR(M_GRAM_LEVEL_2)
                                    << ", " << SSTR(M_GRAM_LEVEL_5) << " ]";
                            throw Exception(msg.str());
                    }
                    if (is_found) {
                        payload_ptr = &ref[found_idx].payload;
                        return true;
                    }
                }

                //Could not compute the probability for the given level, so backing off (recursive)!
                LOG_DEBUG << "Unable to find the " << SSTR(level) << "-Gram  prob for: "
                        << tokensToString(ATrie<N>::m_query_ptr->tokens, level)
                        << ", need to back off!" << END_LOG;
                return false;
            }

            template<TModelLevel N>
            void G2DMapTrie<N>::get_prob_value(const TModelLevel level, TLogProbBackOff & prob) {
                LOG_DEBUG << "Computing probability for a " << level << "-gram " << END_LOG;

                //1. Check which level M-gram we need to get probability for
                if (level > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1
                    LOG_DEBUG << "The level " << level << "-gram max level " << N << END_LOG;

                    //1.1.2. Compute the m-gram hash
                    const uint8_t token_begin_idx = (ATrie<N>::m_query_ptr->level - level);
                    const uint8_t token_end_idx = (ATrie<N>::m_query_ptr->level - 1);
                    TShortId gram_hash = ATrie<N>::m_query_ptr->suffix_hash(token_begin_idx);
                    LOG_DEBUG << "The " << level << "-gram: " << tokensToString(ATrie<N>::m_query_ptr->tokens,
                            token_begin_idx, token_end_idx) << " hash is " << gram_hash << END_LOG;

                    //1.1.3. Search for the bucket
                    const TShortId bucket_idx = get_bucket_id(gram_hash, level);
                    LOG_DEBUG << "The " << level << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4. Search for the probability on the given M-gram level
                    if (level == N) {
                        LOG_DEBUG << "Searching in N grams" << END_LOG;

                        //1.1.4.1 This is an N-gram case
                        const typename TProbBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBucket>(level, m_N_gram_data[bucket_idx], payload_ptr)) {
                            //1.1.4.1.1 The probability is nicely found
                            prob = *payload_ptr;
                            LOG_DEBUG << "The N-gram is found, prob: " << prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The N-gram is not found!" << END_LOG;
                            //1.1.4.1.2 Could not compute the probability for the given level, so backing off (recursive)!
                            ATrie<N>::get_back_off_prob(level - 1, prob);
                        }
                    } else {
                        const TModelLevel mgram_indx = level - ATrie<N>::MGRAM_IDX_OFFSET;
                        LOG_DEBUG << "Searching in " << level << "-grams, array index: " << mgram_indx << END_LOG;

                        //1.1.4.2 This is an M-gram case (1 < M < N))
                        const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                        if (get_payload_from_gram_level<TProbBackOffBucket>(level, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                            //1.1.4.2.1 The probability is nicely found
                            prob = payload_ptr->prob;
                            LOG_DEBUG << "The " << level << "-gram is found, prob: " << prob << END_LOG;
                        } else {
                            LOG_DEBUG << "The " << level << "-gram is not found!" << END_LOG;
                            //1.1.4.2.2 Could not compute the probability for the given level, so backing off (recursive)!
                            ATrie<N>::get_back_off_prob(level - 1, prob);
                        }
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    prob = m_1_gram_data[ATrie<N>::get_end_word_id()].prob;
                    LOG_DEBUG << "Getting the " << level << "-gram prob: " << prob << END_LOG;
                }
            }

            template<TModelLevel N>
            bool G2DMapTrie<N>::get_back_off_weight(const TModelLevel level, TLogProbBackOff & back_off) {
                LOG_DEBUG2 << "Computing back-off for a " << level << "-gram " << END_LOG;

                //Perform a sanity check if needed
                if (DO_SANITY_CHECKS && (level < M_GRAM_LEVEL_1)) {
                    throw Exception("Trying to obtain a back-off weight for level zero!");
                }

                //1. Check which level M-gram we need to get probability for
                if (level > M_GRAM_LEVEL_1) {
                    //1.1. This is the case of the M-gram with M > 1 and clearly M < N
                    LOG_DEBUG << "The level " << level << "-gram max level " << N << END_LOG;

                    //1.1.2. Compute the hash value for the back off M-gram
                    const uint8_t token_begin_idx = (ATrie<N>::m_query_ptr->level - 1) - level;
                    const uint8_t token_end_idx = (ATrie<N>::m_query_ptr->level - 2);
                    TShortId gram_hash = ATrie<N>::m_query_ptr->sub_hash(
                            token_begin_idx, token_end_idx);
                    LOG_DEBUG << "The: " << tokensToString(ATrie<N>::m_query_ptr->tokens, token_begin_idx, token_end_idx)
                            << " " << level << "-gram back-off hash is " << gram_hash << END_LOG;

                    //1.1.3. Search for the bucket
                    const TShortId bucket_idx = get_bucket_id(gram_hash, level);
                    LOG_DEBUG << "The " << level << "-gram hash bucket idx is " << bucket_idx << END_LOG;

                    //1.1.4 This is an M-gram case (1 < M < N))
                    const typename TProbBackOffBucket::TElemType::TPayloadType * payload_ptr = NULL;
                    const TModelLevel mgram_indx = (level - ATrie<N>::MGRAM_IDX_OFFSET);
                    if (get_payload_from_gram_level<TProbBackOffBucket, true>(level, m_M_gram_data[mgram_indx][bucket_idx], payload_ptr)) {
                        //1.1.4.1 The probability is nicely found
                        back_off = payload_ptr->back_off;
                        LOG_DEBUG << "The " << level << "-gram is found, back_off: " << back_off << END_LOG;
                        return true;
                    } else {
                        //The query context id could be determined, but 
                        //the data was not found in the trie.
                        LOG_DEBUG << "Unable to find back-off data for "
                                << level << "-Gram query!" << END_LOG;
                        return false;
                    }
                } else {
                    //1.2. This is the case of a 1-Gram, just get its probability.
                    back_off = m_1_gram_data[ATrie<N>::get_back_off_end_word_id()].back_off;
                    LOG_DEBUG << "Getting the " << level << "-gram back_off: " << back_off << END_LOG;
                    return true;
                }
            }


            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class G2DMapTrie<M_GRAM_LEVEL_MAX>;
        }
    }
}