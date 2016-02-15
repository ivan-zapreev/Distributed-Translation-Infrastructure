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
#include "server/decoder/sentence_data_map.hpp"

#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"
#include "server/lm/lm_configurator.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common::models;

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

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
                         */
                        sentence_decoder(const de_parameters & params)
                        : m_params(params), m_sent_data(NULL),
                        m_lm_query(lm_configurator::allocate_query_proxy()),
                        m_tm_query(tm_configurator::allocate_query_proxy()),
                        m_rm_query(rm_configurator::allocate_query_proxy()) {
                            LOG_DEBUG << "Created a sentence decoder " << (string) m_params << END_LOG;
                        }

                        /**
                         * The basic destructor
                         */
                        ~sentence_decoder() {
                            //Destroy the sentence data
                            if (m_sent_data != NULL) {
                                delete m_sent_data;
                                m_sent_data = NULL;
                            }
                            //Dispose the query objects are they are no longer needed
                            lm_configurator::dispose_query_proxy(m_lm_query);
                            tm_configurator::dispose_query_proxy(m_tm_query);
                            rm_configurator::dispose_query_proxy(m_rm_query);
                        }

                        /**
                         * This is the main method needed to be called for translating a sentence.
                         * @param is_stop the flag that will be set to true in case 
                         *                one needs to abort the translation process.
                         * @param source_sent [in] the source language sentence to translate
                         *                         the source sentence is expected to be
                         *                         tokenized, reduced, and in the lower case.
                         * @param target_sent [out] the resulting target language sentence
                         */
                        inline void translate(acr_bool_flag is_stop,
                                string source_sent, string & target_sent) {
                            //Store the data into the class data members
                            m_source_sent = source_sent;

                            //If the reduced source sentence is not empty then do the translation
                            if (m_source_sent.size() != 0) {

                                //Query the translation model
                                query_translation_model(is_stop);

                                //Return in case we need to stop translating
                                if (is_stop) return;

                                //Query the reordering model
                                query_reordering_model(is_stop);

                                //Return in case we need to stop translating
                                if (is_stop) return;

                                //Perform the translation
                                perform_translation(is_stop, target_sent);
                            }
                        }

                    protected:

                        /**
                         * Allows to set the source sentence, this includes preparing things for decoding
                         */
                        inline void query_translation_model(acr_bool_flag is_stop) {
                            LOG_DEBUG1 << "Got source sentence to set: " << m_source_sent << END_LOG;

                            //Compute the number of tokens in the sentence
                            const size_t num_tokens = std::count(m_source_sent.begin(), m_source_sent.end(), ASCII_SPACE_CHAR) + 1;

                            //Instantiate the sentence info matrix
                            m_sent_data = new sentence_data_map(num_tokens);
                            sentence_data_map & sent_data(*m_sent_data);

                            //Fill in the matrix with the phrases and their uids
                            int32_t col_idx = 0;
                            //Declare the begin and end character index variables
                            size_t ch_b_idx = 0, ch_e_idx = m_source_sent.find_first_of(UTF8_SPACE_STRING);

                            while (ch_e_idx <= std::string::npos && !is_stop) {
                                //Get the appropriate map entry reference
                                sent_data_entry & diag_entry = sent_data[col_idx][col_idx];

                                //Store the phrase begin and end indexes
                                diag_entry.m_begin_idx = ch_b_idx;
                                diag_entry.m_end_idx = ch_e_idx;

                                LOG_DEBUG1 << "Found the new token @ [" << ch_b_idx << "," << ch_e_idx << "): "
                                        << m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx) << END_LOG;

                                //Compute the phrase id
                                diag_entry.m_phrase_uid = get_phrase_uid<true>(m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx));

                                LOG_DEBUG1 << "The token @ [" << ch_b_idx << "," << ch_e_idx << ") uid is: " << diag_entry.m_phrase_uid << END_LOG;

                                //Add the uni-gram phrase to the query
                                m_tm_query.add_source(diag_entry.m_phrase_uid, diag_entry.m_source_entry);

                                LOG_DEBUG1 << "End word: " << m_source_sent.substr(ch_b_idx, ch_e_idx - ch_b_idx) << ", uid: " << diag_entry.m_phrase_uid << END_LOG;

                                //Compute the new phrases and phrase ids for the new column elements,
                                //Note that, the longest phrase length to consider is defined by the
                                //decoding parameters. It is the end word plus several previous.
                                for (int32_t row_idx = max(0, col_idx - m_params.m_max_phrase_len + 1); (row_idx < col_idx) && !is_stop; ++row_idx) {
                                    //Get the previous column entry
                                    sent_data_entry & prev_entry = sent_data[row_idx][col_idx - 1];
                                    //Get the new column entry
                                    sent_data_entry & new_entry = sent_data[row_idx][col_idx];
                                    //Initialize the new entry with data
                                    new_entry.m_begin_idx = prev_entry.m_begin_idx; // All the phrases in the row begin at the same place
                                    new_entry.m_end_idx = diag_entry.m_end_idx; //All the phrases in the column end at the same places
                                    new_entry.m_phrase_uid = get_phrase_uid(prev_entry.m_phrase_uid, diag_entry.m_phrase_uid);

                                    //Add the m-gram phrase to the query
                                    m_tm_query.add_source(new_entry.m_phrase_uid, new_entry.m_source_entry);

                                    LOG_DEBUG1 << "Phrase: " << m_source_sent.substr(new_entry.m_begin_idx, new_entry.m_end_idx - new_entry.m_begin_idx) << ", uid: " << new_entry.m_phrase_uid << END_LOG;
                                }

                                //Check on the stop condition 
                                if (ch_e_idx == std::string::npos) {
                                    break;
                                }

                                //Increment the end word index
                                ++col_idx;

                                //Search for the next delimiter
                                ch_b_idx = ch_e_idx + 1;
                                ch_e_idx = m_source_sent.find_first_of(UTF8_SPACE_STRING, ch_b_idx);
                            }

                            //Execute the query if we are not stopping
                            if (!is_stop) {
                                m_tm_query.execute();
                            }
                        }

                        /**
                         * Allows to query the reordering model based on the set sentence phrases
                         */
                        inline void query_reordering_model(acr_bool_flag is_stop) {
                            //ToDo: Implement
                        }

                        /**
                         * Performs the sentence translation 
                         */
                        inline void perform_translation(acr_bool_flag is_stop, string & target_sent) {
                            //ToDo: Implement

                            const uint32_t time_sec = rand() % 20;
                            for (uint32_t i = 0; i <= time_sec; ++i) {
                                if (is_stop) break;
                                this_thread::sleep_for(chrono::seconds(1));
                            }

                            //ToDo: Remove this temporary plug
                            target_sent = m_source_sent;
                        }

                    private:
                        //Stores the reference to the decoder parameters
                        const de_parameters & m_params;
                        //Stores the source sentence
                        string m_source_sent;
                        //Stores the pointer to the sentence data map
                        sentence_data_map * m_sent_data;
                        //The language mode query proxy
                        lm_query_proxy & m_lm_query;
                        //The language mode query proxy
                        tm_query_proxy & m_tm_query;
                        //The language mode query proxy
                        rm_query_proxy & m_rm_query;
                    };
                }
            }
        }
    }
}

#endif /* SENTENCE_TRANSLATOR_HPP */

