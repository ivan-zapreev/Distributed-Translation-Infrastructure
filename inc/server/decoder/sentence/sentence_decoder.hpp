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
                            m_lm_query(lm_configurator::allocate_query_proxy()),
                            m_tm_query(tm_configurator::allocate_query_proxy()),
                            m_rm_query(rm_configurator::allocate_query_proxy()),
                            m_stack(m_params, m_is_stop, m_sent_data, m_rm_query) {
                                LOG_DEBUG << "Created a sentence decoder " << m_params << END_LOG;
                            }

                            /**
                             * The basic destructor
                             */
                            ~sentence_decoder() {
                                //Dispose the query objects as they are no longer needed
                                lm_configurator::dispose_query_proxy(m_lm_query);
                                tm_configurator::dispose_query_proxy(m_tm_query);
                                rm_configurator::dispose_query_proxy(m_rm_query);
                            }

                            /**
                             * This is the main method needed to be called for translating a sentence.
                             */
                            inline void translate() {
                                //If the reduced source sentence is not empty then do the translation
                                if (m_source_sent.size() != 0) {

                                    //Query the translation model and compute future costs
                                    bootstrap_translate();

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
                                        to_string(MAX_WORDS_PER_SENTENCE));

                                LOG_DEBUG1 << "The sentence: __" << sentence << "___ has " << num_tokens << " tokens" << END_LOG;

                                return num_tokens;
                            }

                            /**
                             * Allows to set the source sentence, this includes preparing things for decoding
                             */
                            inline void bootstrap_translate() {
                                //Fill in the matrix with the phrases and their uids
                                int32_t end_wd_idx = MIN_SENT_WORD_INDEX;
                                //Declare the begin and end character index variables
                                size_t ch_b_idx = 0, ch_e_idx = m_source_sent.find_first_of(UTF8_SPACE_STRING);

                                while (ch_e_idx <= std::string::npos && !m_is_stop) {
                                    //Get the appropriate map entry reference
                                    sent_data_entry & diag_entry = m_sent_data[end_wd_idx][end_wd_idx];

                                    //Store the phrase begin and end character indexes
                                    diag_entry.m_begin_ch_idx = ch_b_idx;
                                    diag_entry.m_end_ch_idx = ch_e_idx;

                                    LOG_DEBUG1 << "Found the new token @ [" << ch_b_idx << "," << ch_e_idx << "): "
                                            << m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx) << END_LOG;

                                    //Compute the phrase id
                                    diag_entry.m_phrase_uid = get_phrase_uid<true>(m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx));

                                    LOG_DEBUG1 << "The token @ [" << ch_b_idx << "," << ch_e_idx << ") uid is: " << diag_entry.m_phrase_uid << END_LOG;

                                    //Add the uni-gram phrase to the query
                                    m_tm_query.execute(diag_entry.m_phrase_uid, diag_entry.m_source_entry);

                                    LOG_DEBUG1 << "End word: " << m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx) << ", uid: " << diag_entry.m_phrase_uid << END_LOG;

                                    //Compute the new phrases and phrase ids for the new column elements,
                                    //Note that, the longest phrase length to consider is defined by the
                                    //decoding parameters. It is the end word plus several previous.
                                    int32_t begin_wd_idx = max(MIN_SENT_WORD_INDEX, end_wd_idx - m_params.m_max_s_phrase_len + 1);
                                    for (; (begin_wd_idx < end_wd_idx) && !m_is_stop; ++begin_wd_idx) {
                                        //Get the previous column entry
                                        sent_data_entry & prev_entry = m_sent_data[begin_wd_idx][end_wd_idx - 1];
                                        //Get the new column entry
                                        sent_data_entry & new_entry = m_sent_data[begin_wd_idx][end_wd_idx];

                                        //Store the phrase begin and end character indexes
                                        new_entry.m_begin_ch_idx = prev_entry.m_begin_ch_idx; // All the phrases in the row begin at the same place
                                        new_entry.m_end_ch_idx = diag_entry.m_end_ch_idx; //All the phrases in the column end at the same place

                                        //Compute the phrase uid
                                        new_entry.m_phrase_uid = combine_phrase_uids(prev_entry.m_phrase_uid, diag_entry.m_phrase_uid);

                                        //Add the m-gram phrase to the query
                                        m_tm_query.execute(new_entry.m_phrase_uid, new_entry.m_source_entry);

                                        //Do logging 
                                        {
                                            string phrase = m_source_sent.substr(new_entry.m_begin_ch_idx, new_entry.m_end_ch_idx - new_entry.m_begin_ch_idx);
                                            LOG_DEBUG1 << "Phrase: " << phrase << ", uid: " << new_entry.m_phrase_uid << END_LOG;
                                        }
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
                                //Perform the decoding process on the level of the stack
                                while (!m_stack.has_finished()) {
                                    //Extend the stack
                                    m_stack.expand();
                                    //Prune the stack
                                    m_stack.prune();
                                }

                                //If we are finished then retrieve the best translation
                                if (!m_is_stop) {
                                    m_stack.get_best_translation(m_target_sent);
                                } else {
                                    m_target_sent = m_source_sent;
                                }
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
                            lm_query_proxy & m_lm_query;
                            //The reference to the translation model query proxy
                            tm_query_proxy & m_tm_query;
                            //The reference to the reordering model query proxy
                            rm_query_proxy & m_rm_query;

                            //Stores the multi-stack
                            multi_stack m_stack;
                        };
                    }
                }
            }
        }
    }
}

#endif /* SENTENCE_TRANSLATOR_HPP */

