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

#include <websocketpp/common/thread.hpp>

#ifndef TRANSLATION_JOB_REQUEST_HPP
#define TRANSLATION_JOB_REQUEST_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents the translation request message.
                     */
                    class translation_job_request {
                    public:
                        typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                        //Stores the minimum translation job id 
                        static constexpr uint32_t MINIMUM_TRANSLATION_JOB_ID = 1;
                        //The delimiter used in the header of the reply message
                        static constexpr char HEADER_DELIMITER = ':';
                        static constexpr char NEW_LINE_HEADER_ENDING = '\n';

                        /**
                         * This is the basic class constructor that accepts the
                         * original client message to parse. This constructor is
                         * to be used on the server to de-serialize the translation
                         * request.
                         * @param message the client translation request to be parsed
                         */
                        translation_job_request(const string & message) {
                            //De-serialize from the message
                            de_serialize(message);
                        }

                        /**
                         * Allows to de-serialize the job request from a string
                         * @param message the string representation of the translation job request
                         */
                        void de_serialize(const string & message) {
                            //Initialize the reader
                            TextPieceReader reader(message.c_str(), message.length());
                            LOG_DEBUG1 << "De-serializing request message: '" << reader.str() << "'" << END_LOG;

                            //The text will contain the read text from the reader
                            TextPieceReader text;

                            //First get the job id
                            if (reader.get_first<HEADER_DELIMITER>(text)) {
                                m_job_id = stoi(text.str());
                                //Second get the source language string
                                if (reader.get_first<NEW_LINE_HEADER_ENDING>(text)) {
                                    m_source_lang = text.str();
                                    //Third get the target language string
                                    if (reader.get_first<NEW_LINE_HEADER_ENDING>(text)) {
                                        m_target_lang = text.str();

                                        //Now the rest is the text to be translated.
                                        m_text = reader.get_rest_str();

                                        LOG_DEBUG << "m_job_id = " << m_job_id << ", m_source_lang = "
                                                << m_source_lang << ", m_target_lang = " << m_target_lang
                                                << ", m_text = " << m_text << END_LOG;
                                    } else {
                                        THROW_EXCEPTION(string("Could not find result code in the job reply header!"));
                                    }
                                } else {
                                    THROW_EXCEPTION(string("Could not find result code in the job reply header!"));
                                }
                            } else {
                                THROW_EXCEPTION(string("Could not find job_id in the job reply header!"));
                            }
                        }

                        /**
                         * Allows to serialize the job request into a string
                         * @return the string representation of the translation job request
                         */
                        const string serialize() {
                            string result = to_string(m_job_id) + HEADER_DELIMITER +
                                    m_source_lang + HEADER_DELIMITER + m_target_lang +
                                    NEW_LINE_HEADER_ENDING + m_text;

                            LOG_DEBUG1 << "Serializing request message: '" << result << "'" << END_LOG;

                            return result;
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation text and source
                         * and target language strings.
                         * @param source_lang the source language string
                         * @param text the text in the source language to translate
                         * @param target_lang the target language string
                         */
                        translation_job_request(const string & source_lang, const string & text, const string & target_lang)
                        : m_job_id(get_next_id()), m_source_lang(source_lang), m_target_lang(target_lang), m_text(text) {
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        const uint32_t get_job_id() const {
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

                    private:
                        //Stores the next job id to be used on the client
                        static uint32_t m_next_job_id;
                        //Stores the synchronization mutex for issuing new ids
                        static websocketpp::lib::mutex m_lock_id;

                        //Stores the translation job id
                        uint32_t m_job_id;
                        //Stores the translation job source language string
                        string m_source_lang;
                        //Stores the translation job target language string
                        string m_target_lang;
                        //Stores the translation job text in the source language.
                        string m_text;

                        /**
                         * Allows to get the next translation job id.
                         * This method is thread safe due to mutex locking.
                         * @return the next job id
                         */
                        static inline uint32_t get_next_id() {
                            scoped_lock guard(m_lock_id);

                            return m_next_job_id++;
                        }
                    };

                    constexpr uint32_t translation_job_request::MINIMUM_TRANSLATION_JOB_ID;
                    constexpr char translation_job_request::HEADER_DELIMITER;
                    constexpr char translation_job_request::NEW_LINE_HEADER_ENDING;

                    uint32_t translation_job_request::m_next_job_id = MINIMUM_TRANSLATION_JOB_ID;
                    websocketpp::lib::mutex translation_job_request::m_lock_id;
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REQUEST_HPP */

