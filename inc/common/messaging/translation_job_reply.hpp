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

#ifndef TRANSLATION_JOB_REPLY_HPP
#define	TRANSLATION_JOB_REPLY_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

                    /**
                     * Stores the translation job result codes, currently
                     * there is just two results possible, the job is
                     * done - OK; or there was some error - ERROR
                     */
                    enum job_result_code {
                        RESULT_OK = 0,
                        RESULT_ERROR = RESULT_OK + 1,
                        size = RESULT_ERROR + 1
                    };

                    /**
                     * This class represents the translation reply message, which
                     * is a translation result for a translation job. This result
                     * can be a text in the target language or it can be an error.
                     */
                    class translation_job_reply {
                    public:

                        /**
                         * This is the basic class constructor that accepts the
                         * original server message to parse. This constructor is
                         * to be used on the client to de-serialize the resulting
                         * message.
                         * @param message the server message to be parsed
                         */
                        translation_job_reply(const string & message) {
                            //De-serialize from the message
                            de_serialize(message);
                        }

                        /**
                         * Allows to de-serialize the job reply from a string
                         * @param message the string representation of the translation job reply
                         */
                        void de_serialize(const string & message) {
                            //ToDo: Implement
                        }

                        /**
                         * Allows to serialize the job reply into a string
                         * @return the string representation of the translation job reply
                         */
                        const string serialize() {
                            return to_string(m_job_id) + string(":") +
                                    to_string(m_code) + string(":") + m_text;
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
                        translation_job_reply(uint32_t job_id, job_result_code code, string text) : m_job_id(job_id), m_code(code), m_text(text) {
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        const uint32_t get_job_id() {
                            return m_job_id;
                        }

                        /**
                         * Allows to get the translation job result code
                         * @return the translation job result code
                         */
                        const job_result_code get_code() {
                            return m_code;
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return the translation job text
                         */
                        const string & get_text() {
                            return m_text;
                        }

                    private:
                        //Stores the translation job id
                        uint32_t m_job_id;
                        //Stores the translation job result code
                        job_result_code m_code;
                        //Stores the translation job result text, the error
                        //message or the text in the target language.
                        string m_text;
                    };
                }
            }
        }
    }
}

#endif	/* TRANSLATION_JOB_REPLY_HPP */

