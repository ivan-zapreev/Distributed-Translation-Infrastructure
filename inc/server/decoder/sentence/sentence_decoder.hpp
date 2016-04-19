/* 
 * File:   sentence_translator.hpp
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
 * Created on February 11, 2016, 4:54 PM
 */

#ifndef SENTENCE_DECODER_HPP
#define SENTENCE_DECODER_HPP

#include <algorithm>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/string_utils.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/decoder/de_parameters.hpp"
#include "server/decoder/sentence/sentence_data_map.hpp"
#include "server/decoder/stack/multi_stack.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common::models;

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::rm;

using namespace uva::smt::bpbd::server::decoder::sentence;
using namespace uva::smt::bpbd::server::decoder::stack;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace sentence {

                        /**
                         * This class represents a sentence translator utility.
                         * It receives a sentence to translate. Performs tokenization,
                         * lowercasing, splitting it into sub-phrases, performs decoding
                         * provides recombines the result into the target sentence.
                         */
                        class sentence_decoder {
                        public:

                            /**
                             * The basic constructor
                             * @param params the reference to the decoder parameters
                             * @param is_stop the flag that will be set to true in case 
                             *                one needs to abort the translation process.
                             * @param source_sent [in] the source language sentence to translate
                             *                         the source sentence is expected to be
                             *                         tokenized, reduced, and in the lower case.
                             * @param target_sent [out] the resulting target language sentence
                             */
                            sentence_decoder(const de_parameters & params, acr_bool_flag is_stop,
                                    const string & source_sent, string & target_sent)
                            : m_params(params), m_is_stop(is_stop), m_source_sent(source_sent),
                            m_target_sent(target_sent), m_sent_data(count_words(m_source_sent)),
                            m_lm_query(lm_configurator::allocate_fast_query_proxy()),
                            m_tm_query(tm_configurator::allocate_query_proxy()),
                            m_rm_query(rm_configurator::allocate_query_proxy()) {
                                LOG_DEBUG << "Created a sentence decoder " << m_params << END_LOG;

                                //Initialize with an empty string
                                m_target_sent = UTF8_EMPTY_STRING;
                            }

                            /**
                             * The basic destructor
                             */
                            ~sentence_decoder() {
                                //Dispose the query objects as they are no longer needed
                                lm_configurator::dispose_fast_query_proxy(m_lm_query);
                                tm_configurator::dispose_query_proxy(m_tm_query);
                                rm_configurator::dispose_query_proxy(m_rm_query);
                            }

                            /**
                             * This is the main method needed to be called for translating a sentence.
                             */
                            inline void translate() {
                                //If the reduced source sentence is not empty then do the translation
                                if (m_source_sent.size() != 0) {

                                    //Query the translation model
                                    query_translation_model();

                                    //Return in case we need to stop translating
                                    if (m_is_stop) return;

                                    //Compute the future costs
                                    compute_futue_costs();

                                    //Return in case we need to stop translating
                                    if (m_is_stop) return;

                                    //Query the reordering model
                                    query_reordering_model();

                                    //Return in case we need to stop translating
                                    if (m_is_stop) return;

                                    //Perform the translation
                                    perform_translation();
                                }
                            }

                        protected:

                            /**
                             * Dynamically initialize the future costs based on the estimates from the TM and LM models.
                             */
                            inline prob_weight & initialize_future_costs(const size_t & start_idx, const size_t & end_idx) {
                                //Get the phrase data entry
                                phrase_data_entry & phrase_data = m_sent_data[start_idx][end_idx];

                                //Get the entry pointer
                                const tm_source_entry * source_entry = phrase_data.m_source_entry;

                                //Get the reference to the future cost
                                prob_weight & cost = phrase_data.future_cost;

                                LOG_DEBUG1 << "Initializing future cost for [" << start_idx << ", " << end_idx << "]" << END_LOG;

                                //Check if the source entry is present, the entry should be there!
                                if (source_entry != NULL) {
                                    LOG_DEBUG1 << "The source entry of phrase [" << start_idx << ", " << end_idx << "] is present." << END_LOG;
                                    //Check if this is a phrase with translation
                                    if (source_entry->has_translations()) {
                                        LOG_DEBUG1 << "The source entry [" << start_idx << ", " << end_idx << "] has translations." << END_LOG;
                                        //Set the value with the pre-computed minimum cost
                                        cost = source_entry->get_min_cost();
                                        LOG_DEBUG1 << "Initialize phrase cost [" << start_idx << ", " << end_idx << "] = " << cost << END_LOG;
                                    } else {
                                        LOG_DEBUG1 << "The source entry of phrase [" << start_idx << ", " << end_idx << "] is UNK translation." << END_LOG;
                                        //Check if this just a single unknown word
                                        if (start_idx == end_idx) {
                                            //For a single UNK word we take its minimum cost which will be actually 
                                            //just the UNK translation cost plus the UNK language model probability
                                            cost = source_entry->get_min_cost();
                                            LOG_DEBUG1 << "Initialize UNK word cost [" << start_idx << ", " << end_idx << "] = " << cost << END_LOG;
                                        } else {
                                            //The undefined log probability value "-1000" is set in the phrase data entry in its constructor!
                                            LOG_DEBUG1 << "Initialize UNK phrase cost [" << start_idx << ", " << end_idx << "] = " << cost << END_LOG;
                                        }
                                    }
                                } else {
                                    //The longer phrases do not have translations, this is normal!
                                    LOG_DEBUG1 << "Initialize TOO-LONG phrase cost [" << start_idx << ", " << end_idx << "] = " << cost << END_LOG;
                                }

                                //Return the reference to the future cost, it will be needed in the caller
                                return cost;
                            }

                            /**
                             * Allows to compute the future costs for the sentence.
                             */
                            inline void compute_futue_costs() {
                                //Obtain the number of words/tokens in the phrase it is equal to the number of dimensions
                                const phrase_length num_words = m_sent_data.get_dim();

                                //Iterate through all the lengths, the minimum length is one word
                                for (phrase_length length = 1; (length <= num_words); ++length) {
                                    LOG_DEBUG1 << "Phrase length: " << length << END_LOG;

                                    //Iterate from the first until the last possible index for the given length
                                    for (phrase_length start_idx = 0; (start_idx <= (num_words - length)); ++start_idx) {
                                        //Compute the end index
                                        const phrase_length end_idx = start_idx + length - 1;
                                        LOG_DEBUG1 << "CFC start/end word idx: " << start_idx << "/" << end_idx << END_LOG;

                                        //Initialize the interval with the TM/LM based value first
                                        //and get the reference to the complete cost then
                                        prob_weight & phrase_cost = initialize_future_costs(start_idx, end_idx);

                                        //Iterate the middle point between start and end indexes
                                        for (phrase_length mid_idx = start_idx; (mid_idx < end_idx); ++mid_idx) {
                                            LOG_DEBUG1 << "CFC middle word idx: " << mid_idx << END_LOG;

                                            //Get the costs of the phrase one and two
                                            const prob_weight ph1_cost = m_sent_data[start_idx][mid_idx].future_cost;
                                            const prob_weight ph2_cost = m_sent_data[mid_idx + 1][end_idx].future_cost;
                                            //Compute the cost of two sub-phrases
                                            const prob_weight sub_cost = ph1_cost + ph2_cost;

                                            LOG_DEBUG1 << "\t cost [" << start_idx << ", " << mid_idx << "] = " << ph1_cost
                                                    << " + cost [" << (mid_idx + 1) << ", " << end_idx << "] = " << ph2_cost
                                                    << " <?> cost [" << start_idx << ", " << end_idx << "] = " << phrase_cost << END_LOG;

                                            //If the sub cost that is a logarithmic value of probability (a negative value) is
                                            //larger than than of the future cost for the entire phrase then use the sub cost.
                                            if (sub_cost > phrase_cost) {
                                                //Set the sum ofthe sub-costs as a new cost
                                                phrase_cost = sub_cost;

                                                LOG_DEBUG << "Changing the cost [" << start_idx << ", " << end_idx
                                                        << "] = [" << start_idx << ", " << mid_idx << "] + ["
                                                        << (mid_idx + 1) << ", " << end_idx << "] = " << phrase_cost << END_LOG;
                                            } else {
                                                LOG_DEBUG << "Keeping the cost [" << start_idx << ", " << end_idx
                                                        << "] = " << phrase_cost << END_LOG;
                                            }

                                            //Check if we need to stop, if yes, then return
                                            if (m_is_stop) return;
                                        }
                                    }
                                }
                            }

                            /**
                             * Allows to count the number of tokens/words in the given sentence
                             * @param sentence the sentence to count the words in
                             * @return the number of words
                             */
                            static inline size_t count_words(const string & sentence) {
                                LOG_DEBUG1 << "Got source sentence: __" << sentence << "___ to count tokens" << END_LOG;

                                //Compute the number of tokens in the sentence
                                const size_t num_tokens = std::count(sentence.begin(), sentence.end(), ASCII_SPACE_CHAR) + 1;

                                //Check the sanity, the used number of words can not be larger than the max
                                ASSERT_CONDITION_THROW((num_tokens > MAX_WORDS_PER_SENTENCE),
                                        string("The number of words in the sentence (") + to_string(num_tokens) +
                                        string(") exceeds the maximum allowed number of words per sentence (") +
                                        to_string(MAX_WORDS_PER_SENTENCE) + string(")"));

                                LOG_DEBUG1 << "The sentence: __" << sentence << "___ has " << num_tokens << " tokens" << END_LOG;

                                return num_tokens;
                            }

                            /**
                             * Allows to set the source sentence, this includes preparing things for decoding
                             */
                            inline void query_translation_model() {
                                //Fill in the matrix with the phrases and their uids
                                int32_t end_wd_idx = MIN_SENT_WORD_INDEX;
                                //Declare the begin and end character index variables
                                size_t ch_b_idx = 0, ch_e_idx = m_source_sent.find_first_of(UTF8_SPACE_STRING);

                                while (ch_e_idx <= std::string::npos) {
                                    //Get the appropriate map entry reference
                                    phrase_data_entry & end_word_data = m_sent_data[end_wd_idx][end_wd_idx];

                                    //Get the token from the source phrase
                                    const string token = m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx);

                                    LOG_DEBUG1 << "Found the new token @ [" << ch_b_idx << ","
                                            << ch_e_idx << "): ___" << token << "___" << END_LOG;

                                    //Store the phrase begin and end character indexes
                                    end_word_data.m_begin_ch_idx = ch_b_idx;
                                    end_word_data.m_end_ch_idx = ch_e_idx;

                                    //Compute the phrase id and store it as well
                                    end_word_data.m_phrase_uid = get_phrase_uid<true>(token);

                                    LOG_DEBUG1 << "The token ___" << token << "___ @ [" << ch_b_idx << ","
                                            << ch_e_idx << ") uid is: " << end_word_data.m_phrase_uid << END_LOG;

                                    LOG_DEBUG1 << "Considering the token [" << end_wd_idx << ", " << end_wd_idx << "] translation." << END_LOG;

                                    //Get the uni-gram phrase (word) translations
                                    m_tm_query.execute(end_word_data.m_phrase_uid, end_word_data.m_source_entry);

                                    LOG_DEBUG1 << "The token ___" << token << "___ @ [" << ch_b_idx << ","
                                            << ch_e_idx << ") HAS" << (end_word_data.m_source_entry->has_translations() ? "" : " NO")
                                            << " translation(s), num entries: " << end_word_data.m_source_entry->num_targets() << END_LOG;

                                    //Compute the new phrases and phrase ids for the new column elements,
                                    //Note that, the longest phrase length to consider is defined by the
                                    //decoding parameters. It is the end word plus several previous.
                                    int32_t begin_wd_idx = max(MIN_SENT_WORD_INDEX, end_wd_idx - m_params.m_max_s_phrase_len + 1);
                                    for (; (begin_wd_idx < end_wd_idx); ++begin_wd_idx) {
                                        LOG_DEBUG1 << "Considering the phrase [" << begin_wd_idx << ", " << end_wd_idx << "] translation." << END_LOG;

                                        //Get the previous column entry
                                        phrase_data_entry & prev_entry = m_sent_data[begin_wd_idx][end_wd_idx - 1];
                                        //Get the new column entry
                                        phrase_data_entry & new_entry = m_sent_data[begin_wd_idx][end_wd_idx];

                                        //Store the phrase begin and end character indexes
                                        new_entry.m_begin_ch_idx = prev_entry.m_begin_ch_idx; // All the phrases in the row begin at the same place
                                        new_entry.m_end_ch_idx = end_word_data.m_end_ch_idx; //All the phrases in the column end at the same place

                                        //Get the phrase for logging
                                        const string phrase = m_source_sent.substr(new_entry.m_begin_ch_idx, new_entry.m_end_ch_idx - new_entry.m_begin_ch_idx);

                                        LOG_DEBUG1 << "The phrase [" << begin_wd_idx << ", " << end_wd_idx << "] is ___" << phrase << "___" << END_LOG;

                                        //Compute the phrase uid
                                        new_entry.m_phrase_uid = combine_phrase_uids(prev_entry.m_phrase_uid, end_word_data.m_phrase_uid);

                                        LOG_DEBUG1 << "The phrase ___" << phrase << "___ uid = combine(" << prev_entry.m_phrase_uid
                                                << "," << end_word_data.m_phrase_uid << ") = " << new_entry.m_phrase_uid << END_LOG;

                                        //Add the m-gram phrase to the query
                                        m_tm_query.execute(new_entry.m_phrase_uid, new_entry.m_source_entry);

                                        LOG_DEBUG1 << "Phrase: ___" << phrase << "___ uid: " << new_entry.m_phrase_uid << " HAS"
                                                << (new_entry.m_source_entry->has_translations() ? "" : " NO") << " translation(s),"
                                                << " num entries: " << new_entry.m_source_entry->num_targets() << END_LOG;

                                        //Check if we need to stop, if yes, then return
                                        if (m_is_stop) return;
                                    }

                                    //Check on the stop condition 
                                    if (ch_e_idx == std::string::npos) {
                                        break;
                                    }

                                    //Increment the end word index
                                    ++end_wd_idx;

                                    //Search for the next delimiter
                                    ch_b_idx = ch_e_idx + 1;
                                    ch_e_idx = m_source_sent.find_first_of(UTF8_SPACE_STRING, ch_b_idx);
                                }
                            }

                            /**
                             * Allows to query the reordering model based on the set sentence phrases
                             */
                            inline void query_reordering_model() {
                                //Declare the source-target phrase uid container
                                vector<phrase_uid> st_uids;

                                //Obtain the list of source-target ids available
                                if (!m_is_stop) {
                                    m_tm_query.get_st_uids(st_uids);
                                }

                                //Execute the reordering model query
                                if (!m_is_stop) {
                                    m_rm_query.execute(st_uids);
                                }
                            }

                            /**
                             * Performs the sentence translation 
                             */
                            inline void perform_translation() {
                                //Depending on the stack template parameters do the thing
                                if (m_params.m_distortion != 0) {
                                    if (m_params.m_num_alt_to_keep != 0) {
                                        perform_translation<true, true>();
                                    } else {
                                        perform_translation<true, false>();
                                    }
                                } else {
                                    if (m_params.m_num_alt_to_keep != 0) {
                                        perform_translation<false, true>();
                                    } else {
                                        perform_translation<false, false>();
                                    }
                                }
                            }

                        protected:

                            /**
                             * Performs the sentence translation 
                             */
                            template<bool is_dist, bool is_alt_trans>
                            inline void perform_translation() {
                                //Stores the multi-stack
                                multi_stack_templ<is_dist, is_alt_trans> m_stack(m_params, m_is_stop, m_source_sent, m_sent_data, m_rm_query, m_lm_query);

                                //Extend the stack, here we do everything in one go
                                //Including expanding, pruning and recombination
                                m_stack.expand();

                                //If we are finished then retrieve the best 
                                //translation. If we have stopped then nothing.
                                m_stack.get_best_trans(m_target_sent);
                            }

                        private:
                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;
                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;

                            //Stores the reference to the source sentence
                            const string & m_source_sent;
                            //Stores the reference to the target sentence
                            string & m_target_sent;

                            //Stores the pointer to the sentence data map
                            sentence_data_map m_sent_data;

                            //The reference to the translation model query proxy
                            lm_fast_query_proxy & m_lm_query;
                            //The reference to the translation model query proxy
                            tm_query_proxy & m_tm_query;
                            //The reference to the reordering model query proxy
                            rm_query_proxy & m_rm_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* SENTENCE_TRANSLATOR_HPP */

