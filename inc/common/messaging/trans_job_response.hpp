/* 
 * File:   translation_job_reply.hpp
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

#include <string>
#include <sstream>
#include <iostream>

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"
#include "common/utils/file/TextPieceReader.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_code.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::file;
using namespace uva::smt::decoding::common::messaging;

#ifndef TRANSLATION_JOB_REPLY_HPP
#define TRANSLATION_JOB_REPLY_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

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

                        /**
                         * This is the basic class constructor that accepts the
                         * original server message to parse. This constructor is
                         * to be used on the client to de-serialize the resulting
                         * message.
                         * @param message the server message to be parsed
                         */
                        trans_job_response(const string & message) {
                            //De-serialize from the message
                            de_serialize(message);
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation result code and 
                         * the text.
                         * @param job_id the client-issued id of the translation job 
                         * @param code the translation job result code
                         * @param text the translation job result text, either
                         * the translated text or the error message corresponding
                         * to the error code
                         */
                        trans_job_response(const job_id_type job_id, const trans_job_code code,
                                const string & text) : m_job_id(job_id), m_code(code), m_text(text) {
                        }

                        /**
                         * Allows to de-serialize the job reply from a string
                         * @param message the string representation of the translation job reply
                         */
                        void de_serialize(const string & message) {
                            //Initialize the reader
                            TextPieceReader reader(message.c_str(), message.length());
                            LOG_DEBUG1 << "De-serializing reply message: '" << reader.str() << "'" << END_LOG;

                            //The text will contain the read text from the reader
                            TextPieceReader text;

                            //First get the job id
                            if (reader.get_first<HEADER_DELIMITER>(text)) {
                                m_job_id = stoi(text.str());
                                //Second get the result code
                                if (reader.get_first<NEW_LINE_HEADER_ENDING>(text)) {
                                    m_code = (trans_job_code) stoi(text.str());

                                    //Now the rest is the translated text or the error message
                                    m_text = reader.get_rest_str();

                                    LOG_DEBUG << "\nm_job_id = " << m_job_id << ", m_code = " << m_code << ", m_text = \n" << m_text << END_LOG;
                                } else {
                                    THROW_EXCEPTION(string("Could not find result code in the job reply header!"));
                                }
                            } else {
                                THROW_EXCEPTION(string("Could not find job_id in the job reply header!"));
                            }
                        }

                        /**
                         * Allows to serialize the job reply into a string
                         * @return the string representation of the translation job reply
                         */
                        const string serialize() {
                            string result = to_string(m_job_id) + HEADER_DELIMITER +
                                    to_string(m_code) + NEW_LINE_HEADER_ENDING + m_text;

                            LOG_DEBUG1 << "Serializing reply message: '" << result << "'" << END_LOG;

                            return result;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        const job_id_type get_job_id() const {
                            return m_job_id;
                        }

                        /**
                         * Allows to check whether the job id is defined, is not
                         * equal to job_id::UNDEFINED_JOB_ID;
                         * @return true if the job id is defined, otherwise false
                         */
                        const bool is_job_id_defined() const {
                            return (m_job_id != job_id::UNDEFINED_JOB_ID);
                        }

                        /**
                         * Allows to check if the reply is good, i.e. contains the translated text and not the error message
                         * @return true if the reply is good and contains the translated text.
                         */
                        const bool is_good() const {
                            return (m_code == trans_job_code::RESULT_OK);
                        }

                        /**
                         * Allows to get the translation job result code
                         * @return the translation job result code
                         */
                        const trans_job_code get_code() const {
                            return m_code;
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return the translation job text
                         */
                        const string & get_text() const {
                            return m_text;
                        }

                    private:
                        //Stores the translation job id
                        job_id_type m_job_id;
                        //Stores the translation job result code
                        trans_job_code m_code;
                        //Stores the translation job result text, the error
                        //message or the text in the target language.
                        string m_text;
                    };

                    constexpr char trans_job_response::HEADER_DELIMITER;
                    constexpr char trans_job_response::NEW_LINE_HEADER_ENDING;
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REPLY_HPP */

