/* 
 * File:   trans_job_resp_in.hpp
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
 * Created on June 23, 2016, 3:38 PM
 */

#ifndef TRANS_JOB_RESP_IN_HPP
#define TRANS_JOB_RESP_IN_HPP

#include "common/messaging/trans_job_resp.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/job_id.hpp"

#include "client/messaging/trans_sent_data_in.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents an incoming translation job response
                     */
                    class trans_job_resp_in : public trans_job_resp {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        trans_job_resp_in(incoming_msg * inc_msg)
                        : trans_job_resp(), m_inc_msg(inc_msg), m_sent_data() {
                            //Get the sentence data array
                            const Value & sent_data_arr = m_inc_msg->get_json()[TARGET_DATA_FIELD_NAME];
                            //Store the iterator for the beginning of this array
                            m_sent_data_iter = sent_data_arr.Begin();
                            //Store the iterator for the end of this array
                            m_sent_data_end_iter = sent_data_arr.End();
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_resp_in() {
                            //Delete the incoming message, it must always be present
                            delete m_inc_msg;
                        }

                        /**
                         * Allows to iterate through the sentence data objects.
                         * This is meant to be done one time only
                         * @return a pointer to the sentence data object or NULL if we reached the end of the list
                         */
                        inline const trans_sent_data_in * next_send_data() {
                            if (m_sent_data_iter != m_sent_data_end_iter) {
                                //Set the new object into the sentence data
                                m_sent_data.set_sent_data(*m_sent_data_iter);
                                //Increment the iterator for the next retrieval
                                ++m_sent_data_iter;
                                //Return the pointer to the wrapper object
                                return &m_sent_data;
                            } else {
                                //We've reached the end so we return NULL.
                                return NULL;
                            }
                        }

                        /**
                         * Allows to get the job id
                         * @return the job id
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
                         * Allows to retrieve the incoming message
                         * @return the incoming message
                         */
                        inline const incoming_msg * get_message() const {
                            return m_inc_msg;
                        }

                    private:
                        //Stores the pointer to the incoming message storing
                        //the response data, this pointer must not be NULL
                        incoming_msg * m_inc_msg;
                        //Stores the sentence data wrapper object
                        trans_sent_data_in m_sent_data;
                        //Stores the end iterator for the sentence data array
                        Value::ConstValueIterator m_sent_data_end_iter;
                        //Stores the iterator for the sentence data array
                        Value::ConstValueIterator m_sent_data_iter;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_RESP_IN_HPP */

