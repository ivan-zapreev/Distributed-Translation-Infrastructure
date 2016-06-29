/* 
 * File:   trans_job_req_in.hpp
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
 * Created on June 23, 2016, 3:37 PM
 */

#ifndef TRANS_JOB_REQ_IN_HPP
#define TRANS_JOB_REQ_IN_HPP

#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/trans_job_req.hpp"
#include "common/messaging/trans_job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    /**
                     * This class represents the translation job request received by the server.
                     */
                    class trans_job_req_in : public trans_job_req {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        trans_job_req_in(const incoming_msg * inc_msg)
                        : trans_job_req(), m_inc_msg(inc_msg) {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_req_in() {
                            //Destroy the incoming message, the pointer must not be NULL
                            delete m_inc_msg;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline job_id_type get_job_id() const {
                            return m_inc_msg->get_value(JOB_ID_FIELD_NAME);
                        }

                        /**
                         * Allows to get the translation job source language
                         * @return the translation job source language, lowercased
                         */
                        inline string get_source_lang() const {
                            return m_inc_msg->get_value(SOURCE_LANG_FIELD_NAME);
                        }

                        /**
                         * Allows to get the translation job target language
                         * @return the translation job target language, lowercased
                         */
                        inline string get_target_lang() const {
                            return m_inc_msg->get_value(TARGET_LANG_FIELD_NAME);
                        }

                        /**
                         * Allows to check whether the client has requested the translation information
                         * @return true if the translation information is requested, otherwise false
                         */
                        inline bool is_trans_info() const {
                            return m_inc_msg->get_value(IS_TRANS_INFO_FIELD_NAME);
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return an array of sentences to be translated, encapsulated in a json object
                         */
                        inline const json & get_source_text() const {
                            return m_inc_msg->get_value(SOURCE_SENTENCES_FIELD_NAME);
                        }

                    private:
                        //Stores the pointer to the incoming message
                        const incoming_msg * m_inc_msg;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_REQ_IN_HPP */

