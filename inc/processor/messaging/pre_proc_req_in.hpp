/* 
 * File:   pre_proc_req_in.hpp
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
 * Created on July 26, 2016, 10:43 AM
 */

#ifndef PRE_PROC_REQ_IN_HPP
#define	PRE_PROC_REQ_IN_HPP

#include "common/messaging/language_registry.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/pre_proc_req.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                namespace messaging {

                    /**
                     * This class represents the incoming text post-process request.
                     */
                    class pre_proc_req_in : public pre_proc_req {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        pre_proc_req_in(const incoming_msg * inc_msg)
                        : pre_proc_req(), m_inc_msg(inc_msg) {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~pre_proc_req_in() {
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
                         * Allows to get this task id
                         * @return the client-issued job id
                         */
                        inline job_id_type get_task_id() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[TASK_ID_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the number of job's tasks
                         * @return the number of job's tasks
                         */
                        inline job_id_type get_num_tasks() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[NUM_TASKS_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the source language for the task,
                         * it should be equal for all the job's tasks
                         * @return the pre-processor job source language, lowercased
                         */
                        inline string get_source_lang() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[SOURCE_LANG_FIELD_NAME].GetString();
                        }

                        /**
                         * Allows to get the translation job source language uid.
                         * In case the language is unknown to the local language
                         * registry or the source language is <auto> then an
                         * unknown language id is returned:
                         *  language_registry::UNKNONW_LANGUAGE_ID
                         * @return the translation job source language uid
                         */
                        inline language_uid get_source_lang_uid() const {
                            const string lang = get_source_lang();
                            return language_registry::get_uid(lang);
                        }

                        /**
                         * Allows to get the pre-processor task source text
                         * @return the pre-processor task source text
                         */
                        inline string get_source_text() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[SOURCE_TEXT_FIELD_NAME].GetString();
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

#endif	/* PRE_PROC_REQ_IN_HPP */

