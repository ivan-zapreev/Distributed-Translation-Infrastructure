/* 
 * File:   trans_job_response.hpp
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
 * Created on January 18, 2016, 5:02 PM
 */

#ifndef TRANSLATION_JOB_REPLY_HPP
#define TRANSLATION_JOB_REPLY_HPP

#include <string>
#include <sstream>
#include <iostream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"

#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_code.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    //Do a forward definition of the class
                    class trans_job_response;

                    //Define the pointer type for the job response
                    typedef trans_job_response * trans_job_response_ptr;

                    /**
                     * This class represents the translation reply message, which
                     * is a translation result for a translation job. This result
                     * can be a text in the target language or it can be an error.
                     */
                    class trans_job_response {
                    public:
                        //The delimiter used in the header of the reply message
                        static constexpr char HEADER_DELIMITER = ':';
                        static constexpr char NEW_LINE_HEADER_ENDING = '\n';

                        //The begin of the translation job response message
                        static const string TRANS_JOB_RESPONSE_PREFIX;

                        /**
                         * This is the basic class constructor that accepts the
                         * original server message to parse. This constructor is
                         * to be used on the client to de-serialize the resulting
                         * message.
                         * @param data the server message to be parsed
                         */
                        trans_job_response(const string & data) {
                            //De-serialize from the message
                            de_serialize(data);
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation result code and 
                         * the text.
                         * @param job_id the client-issued id of the translation job 
                         * @param status_code the translation job result code
                         * @param status_msg the translation job status message
                         * @param target_text the translation job result text, either
                         * the translated text or the error message corresponding
                         * to the error code
                         */
                        trans_job_response(const job_id_type job_id, const trans_job_code code,
                                const string & status_msg, const string & target_text)
                        : m_job_id(job_id),
                        m_status_code(code), m_status_msg(status_msg),
                        m_target_text(target_text) {
                        }

                        /**
                         * Allows to de-serialize the job reply from a string
                         * @param message the string representation of the translation job reply
                         */
                        void de_serialize(const string & message) {
                            //Initialize the reader
                            text_piece_reader reader(message.c_str(), message.length());
                            LOG_DEBUG3 << "De-serializing reply message (" << &message << "): '" << reader.str() << "'" << END_LOG;

                            //The text will contain the read text from the reader
                            text_piece_reader text;

                            //Skip the translation job response prefix
                            if (reader.get_first<HEADER_DELIMITER>(text)) {
                                //Get the job id
                                if (reader.get_first<HEADER_DELIMITER>(text)) {
                                    LOG_DEBUG1 << "Message " << &message << ", read the job id: " << text.str() << END_LOG;
                                    m_job_id = stoi(text.str());
                                    //Second get the result code
                                    if (reader.get_first<NEW_LINE_HEADER_ENDING>(text)) {
                                        LOG_DEBUG1 << "Message " << &message << ", read the job code: " << text.str() << END_LOG;
                                        m_status_code = trans_job_code(stoi(text.str()));

                                        //Now the rest is the translated text or the error message
                                        m_target_text = reader.get_rest_str();

                                        LOG_DEBUG << "Received message " << &message << ", \nm_job_id = " << m_job_id
                                                << ", m_code = " << m_status_code << ", m_text = \n" << m_target_text << END_LOG;
                                    } else {
                                        THROW_EXCEPTION(string("Could not find result code in the job reply header!"));
                                    }
                                } else {
                                    THROW_EXCEPTION(string("Could not find job_id in the job reply header!"));
                                }
                            } else {
                                THROW_EXCEPTION(string("Could not skip the translation job response prefix!"));
                            }
                        }

                        /**
                         * Allows to serialize the job reply into a string
                         * @return the string representation of the translation job reply
                         */
                        inline const string serialize() {
                            string result = TRANS_JOB_RESPONSE_PREFIX + to_string(m_job_id) + HEADER_DELIMITER +
                                    to_string(m_status_code.val()) + NEW_LINE_HEADER_ENDING + m_target_text;

                            LOG_DEBUG1 << "Serializing reply message: '" << result << "'" << END_LOG;

                            return result;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline job_id_type get_job_id() const {
                            return m_job_id;
                        }

                        /**
                         * Allows to check whether the job id is defined, is not
                         * equal to job_id::UNDEFINED_JOB_ID;
                         * @return true if the job id is defined, otherwise false
                         */
                        inline bool is_job_id_defined() const {
                            return (m_job_id != job_id::UNDEFINED_JOB_ID);
                        }

                        /**
                         * Allows to check if the reply is good, i.e. contains the translated text and not the error message
                         * @return true if the reply is good and contains the translated text.
                         */
                        inline bool is_good() const {
                            return (m_status_code == trans_job_code::RESULT_OK);
                        }

                        /**
                         * Allows to get the translation job result code
                         * @return the translation job result code
                         */
                        inline trans_job_code get_status_code() const {
                            return m_status_code;
                        }

                        /**
                         * Allows to get the translation job status message
                         * @return the translation job status message
                         */
                        inline const string & get_status_msg() const {
                            return m_status_msg;
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return the translation job text
                         */
                        inline const string & get_target_text() const {
                            return m_target_text;
                        }

                    private:
                        //Stores the translation job id
                        job_id_type m_job_id;
                        //Stores the translation job result code
                        trans_job_code m_status_code;
                        //Stores the translation job status message
                        string m_status_msg;
                        //Stores the translation job result text, the error
                        //message or the text in the target language.
                        string m_target_text;
                    };
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REPLY_HPP */

