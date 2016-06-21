/* 
 * File:   trans_job_request.hpp
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
 * Created on January 18, 2016, 5:05 PM
 */

#ifndef TRANS_JOB_REQUEST_HPP
#define TRANS_JOB_REQUEST_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"

#include "common/messaging/json_msg.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::file;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    //Declare the translation job request pointer type
                    class trans_job_request;
                    typedef trans_job_request * trans_job_request_ptr;

                    /**
                     * This class represents the translation request message.
                     */
                    class trans_job_request {
                    public:
                        //Stores the job id attribute name
                        static const string JOB_ID_NAME;
                        //Stores the source language attribute name
                        static const string SOURCE_LANG_NAME;
                        //Stores the target language attribute name
                        static const string TARGET_LANG_NAME;
                        //Stores the translation info flag attribute name
                        static const string TRANS_INFO_FLAG_NAME;
                        //Stores the source sentences attribute name
                        static const string SOURCE_SENTENCES_NAME;

                        /**
                         * This is the basic class. This constructor is to be used
                         * when a request is received.
                         * @param req_msg a reference to the JSON object storing the request data.
                         */
                        trans_job_request(const json_msg & req_msg) : m_msg(), m_act_msg(req_msg) {
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation text and source
                         * and target language strings.
                         * @param job_id the translation job id
                         * @param source_lang the source language string
                         * @param source_text the text in the source language to translate
                         * @param target_lang the target language string
                         * @param is_trans_info true if the client should requests the translation info from the server
                         */
                        trans_job_request(const job_id_type job_id, const string & source_lang,
                                vector<string> & source_text, const string & target_lang, const bool is_trans_info)
                        : m_msg(msg_type::MESSAGE_TRANS_JOB_REQ), m_act_msg(m_msg) {
                            m_msg.m_json_obj[JOB_ID_NAME] = job_id;
                            m_msg.m_json_obj[SOURCE_LANG_NAME] = source_lang;
                            m_msg.m_json_obj[TARGET_LANG_NAME] = target_lang;
                            m_msg.m_json_obj[TRANS_INFO_FLAG_NAME] = is_trans_info;
                            m_msg.m_json_obj[SOURCE_SENTENCES_NAME] = source_text;

                            LOG_DEBUG << "Translation job request, job id: " << job_id
                                    << " source language: " << source_lang
                                    << " target language: " << target_lang
                                    << " translation info flag: " << is_trans_info << END_LOG;
                        }

                        /**
                         * Allows to serialize the job request into a string
                         * @return the string representation of the translation job request
                         */
                        inline const string serialize() const {
                            string result = m_act_msg.serialize();
                            LOG_DEBUG1 << "Serializing request message: '" << result << "'" << END_LOG;
                            return result;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline const job_id_type get_job_id() const {
                            return m_act_msg.get_value<job_id_type>(JOB_ID_NAME);
                        }

                        /**
                         * Allows to get the translation job source language
                         * @return the translation job source language
                         */
                        inline const string get_source_lang() const {
                            return m_act_msg.get_value<string>(SOURCE_LANG_NAME);
                        }

                        /**
                         * Allows to get the translation job target language
                         * @return the translation job target language
                         */
                        inline const string get_target_lang() const {
                            return m_act_msg.get_value<string>(TARGET_LANG_NAME);
                        }

                        /**
                         * Allows to check whether the client has requested the translation information
                         * @return true if the translation information is requested, otherwise false
                         */
                        inline const bool is_trans_info() const {
                            return m_act_msg.get_value<bool>(TRANS_INFO_FLAG_NAME);
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return the translation job text
                         */
                        inline json::array_t get_source_text() const {
                            return m_act_msg.get_value<json::array_t>(SOURCE_SENTENCES_NAME);
                        }

                    private:
                        //Stores the the JSON message object
                        json_msg m_msg;
                        //Stores the reference to the actual JSON message to work with
                        const json_msg & m_act_msg;
                    };
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REQUEST_HPP */

