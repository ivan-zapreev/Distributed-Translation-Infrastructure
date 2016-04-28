/* 
 * File:   translation_job_request.hpp
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
                        //The delimiter used in the header of the reply message
                        static constexpr char HEADER_DELIMITER = ':';
                        static constexpr char NEW_LINE_HEADER_ENDING = '\n';
                        static constexpr char TEXT_SENTENCE_DELIMITER = '\n';

                        /**
                         * This is the basic class constructor that accepts the
                         * original client message to parse. This constructor is
                         * to be used on the server to de-serialize the translation
                         * request.
                         */
                        trans_job_request() : m_session_id(session_id::UNDEFINED_SESSION_ID) {
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation text and source
                         * and target language strings.
                         * @param job_id the translation job id
                         * @param source_lang the source language string
                         * @param text the text in the source language to translate
                         * @param target_lang the target language string
                         * @param is_trans_info true if the client should requests the translation info from the server
                         */
                        trans_job_request(const job_id_type job_id, const string & source_lang,
                                const string & text, const string & target_lang,
                                const bool is_trans_info)
                        : m_session_id(session_id::UNDEFINED_SESSION_ID), m_job_id(job_id),
                        m_source_lang(source_lang), m_target_lang(target_lang), m_text(text),
                        m_is_trans_info(is_trans_info) {
                            LOG_DEBUG << "Created a translation job request with job id: " << m_job_id << " and text:\n" << m_text << END_LOG;
                        }

                        /**
                         * Allows to de-serialize the job request from a string
                         * @param request the string representation of the translation job request
                         */
                        void de_serialize(const string & request) {
                            try {
                                //Initialize the reader
                                text_piece_reader reader(request.c_str(), request.length());
                                LOG_DEBUG1 << "De-serializing request message: '" << reader.str() << "'" << END_LOG;

                                //The text will contain the read text from the reader
                                text_piece_reader text;

                                //Get the job id
                                if (reader.get_first<HEADER_DELIMITER>(text)) {
                                    m_job_id = stoi(text.str());
                                    //Get the source language string
                                    if (reader.get_first<HEADER_DELIMITER>(text)) {
                                        m_source_lang = text.str();
                                        //Get the target language string
                                        if (reader.get_first<HEADER_DELIMITER>(text)) {
                                            m_target_lang = text.str();
                                            //Get the translation clues flag
                                            if (reader.get_first<NEW_LINE_HEADER_ENDING>(text)) {
                                                m_is_trans_info = (stoi(text.str()) != 0);

                                                //Now the rest is the text to be translated.
                                                m_text = reader.get_rest_str();

                                                LOG_DEBUG << "\nm_job_id = " << m_job_id << ", m_source_lang = "
                                                        << m_source_lang << ", m_target_lang = " << m_target_lang
                                                        << ", m_text = \n" << m_text << END_LOG;
                                            } else {
                                                THROW_EXCEPTION(string("Could not find the translation clue flag in the job request header!"));
                                            }
                                        } else {
                                            THROW_EXCEPTION(string("Could not find the target language in the job request header!"));
                                        }
                                    } else {
                                        THROW_EXCEPTION(string("Could not find the source language in the job request header!"));
                                    }
                                } else {
                                    THROW_EXCEPTION(string("Could not find job_id in the job request header!"));
                                }
                            } catch (invalid_argument & ex1) {
                                THROW_EXCEPTION(string("Error invalid_argument for job request!"));
                            } catch (out_of_range & ex1) {
                                THROW_EXCEPTION(string("Error out_of_range for job request!"));
                            }
                        }

                        /**
                         * Allows to serialize the job request into a string
                         * @return the string representation of the translation job request
                         */
                        const string serialize() const {
                            string result = to_string(m_job_id) + HEADER_DELIMITER +
                                    m_source_lang + HEADER_DELIMITER +
                                    m_target_lang + HEADER_DELIMITER +
                                    to_string(m_is_trans_info) +
                                    NEW_LINE_HEADER_ENDING + m_text;

                            LOG_DEBUG1 << "Serializing request message: '" << result << "'" << END_LOG;

                            return result;
                        }

                        /**
                         * Allows to set the translation session id. This method to be
                         * used on the client, for the sake of storing the session id
                         * by the translation job request class.
                         * @param session_id the session id issued by the server
                         */
                        void set_session_id(const session_id_type session_id) {
                            m_session_id = session_id;
                        }

                        /**
                         * Allows to get the translation session id. This method to be
                         * used on the client, for the sake of storing the session id
                         * by the translation job request class.
                         * @return the session id issued by the server
                         */
                        const session_id_type get_session_id() const {
                            return m_session_id;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        const job_id_type get_job_id() const {
                            return m_job_id;
                        }

                        /**
                         * Allows to get the translation job source language
                         * @return the translation job source language
                         */
                        const string get_source_lang() const {
                            return m_source_lang;
                        }

                        /**
                         * Allows to get the translation job target language
                         * @return the translation job target language
                         */
                        const string get_target_lang() const {
                            return m_target_lang;
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

                        /**
                         * Allows to check whether the client has requested the translation information
                         * @return true if the translation information is requested, otherwise false
                         */
                        const bool is_trans_info() const {
                            return m_is_trans_info;
                        }

                    private:
                        //Stores the session id, undefined in the first place and always on the client
                        session_id_type m_session_id;
                        //Stores the translation job id
                        job_id_type m_job_id;
                        //Stores the translation job source language string
                        string m_source_lang;
                        //Stores the translation job target language string
                        string m_target_lang;
                        //Stores the translation job text in the source language.
                        string m_text;
                        //Stores the translation info flag
                        bool m_is_trans_info;
                    };

                    constexpr char trans_job_request::HEADER_DELIMITER;
                    constexpr char trans_job_request::NEW_LINE_HEADER_ENDING;
                    constexpr char trans_job_request::TEXT_SENTENCE_DELIMITER;
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REQUEST_HPP */

