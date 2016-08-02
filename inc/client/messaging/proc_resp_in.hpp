/* 
 * File:   proc_resp_in.hpp
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
 * Created on August 2, 2016, 9:19 AM
 */

#ifndef PROC_RESP_IN_HPP
#define PROC_RESP_IN_HPP

#include "common/messaging/proc_resp.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {
                    
                    /**
                     * This class represents the incoming text processor response.
                     */
                    class proc_resp_in : public proc_resp {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        proc_resp_in(const incoming_msg * inc_msg)
                        : proc_resp(), m_inc_msg(inc_msg) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~proc_resp_in() {
                            //Destroy the incoming message, the pointer must not be NULL
                            delete m_inc_msg;
                        }

                        /**
                         * Allows to get the translation task result code
                         * @return the translation task result code
                         */
                        inline status_code get_status_code() const {
                            const Document & json = m_inc_msg->get_json();
                            return status_code(json[STAT_CODE_FIELD_NAME].GetInt());
                        }

                        /**
                         * Allows to get the translation task status message
                         * @return the translation task status message
                         */
                        inline string get_status_msg() const {
                            const Document & json = m_inc_msg->get_json();
                            
                            return json[STAT_MSG_FIELD_NAME].GetString();
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
                         * Allows to get this text piece index, the index starts with
                         * zero and correspond to the text piece index in the job.
                         * @return the task index represented by this request
                         */
                        inline job_id_type get_chunk_idx() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[CHUNK_IDX_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the number of job's text pieces
                         * @return the number of job's text pieces
                         */
                        inline job_id_type get_num_chunks() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[NUM_CHUNKS_FIELD_NAME].GetUint64();
                        }

                        /**
                         * Allows to get the language for the task,
                         * it should be equal for all the job's tasks
                         * @return the processor job language, lowercased
                         */
                        inline string get_language() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[LANG_FIELD_NAME].GetString();
                        }

                        /**
                         * Allows to get the processor task text
                         * @return the processor task text
                         */
                        inline string get_chunk() const {
                            const Document & json = m_inc_msg->get_json();
                            return json[CHUNK_FIELD_NAME].GetString();
                        }

                    protected:
                        //Stores the pointer to the incoming message
                        const incoming_msg * m_inc_msg;

                    };
                    
                    //Typedef the pointer to the request
                    typedef proc_resp_in * proc_resp_in_ptr;
                }
            }
        }
    }
}

#endif /* PROC_RESP_IN_HPP */

