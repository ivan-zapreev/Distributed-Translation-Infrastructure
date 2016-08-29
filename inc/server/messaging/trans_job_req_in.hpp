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

#include "common/messaging/language_registry.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/trans_job_req.hpp"
#include "common/messaging/job_id.hpp"

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
                        trans_job_req_in(incoming_msg * inc_msg)
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
                            const Document & json = m_inc_msg->get_json();
                            return json[JOB_ID_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline void set_job_id(job_id_type job_id) {
                            Document & json = m_inc_msg->get_json();
                            json[JOB_ID_FIELD_NAME] = job_id;
                        }
                        
                        /**
                         * Allows to get the translation job priority
                         * @return the translation job priority
                         */
                        inline int32_t get_priority() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[PRIORITY_NAME].GetInt();
                        }

                        /**
                         * Allows to get the translation job source language
                         * @return the translation job source language, lowercased
                         */
                        inline string get_source_lang() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[SOURCE_LANG_FIELD_NAME].GetString();
                        }

                        /**
                         * Allows to get the translation job source language uid
                         * @return the translation job source language uid
                         */
                        inline language_uid get_source_lang_uid() const {
                            const string lang = get_source_lang();
                            return language_registry::get_uid(lang);
                        }

                        /**
                         * Allows to get the translation job target language
                         * @return the translation job target language, lowercased
                         */
                        inline string get_target_lang() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[TARGET_LANG_FIELD_NAME].GetString();
                        }

                        /**
                         * Allows to get the translation job target language uid
                         * @return the translation job target language uid
                         */
                        inline language_uid get_target_lang_uid() const {
                            const string lang = get_target_lang();
                            return language_registry::get_uid(lang);
                        }

                        /**
                         * Allows to check whether the client has requested the translation information
                         * @return true if the translation information is requested, otherwise false
                         */
                        inline bool is_trans_info() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[IS_TRANS_INFO_FIELD_NAME].GetBool();
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return an array of sentences to be translated, encapsulated in a json object
                         */
                        inline const Value & get_source_text() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[SOURCE_SENTENCES_FIELD_NAME];
                        }
                        
                        /**
                         * Allows to retrieve the incoming message
                         * @return the incoming message
                         */
                        inline const incoming_msg * get_message() const {
                            return m_inc_msg;
                        }

                    private:
                        //Stores the pointer to the incoming message
                        incoming_msg * m_inc_msg;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_REQ_IN_HPP */

