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
#include "common/messaging/trans_job_id.hpp"

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
                    class trans_job_resp_in : public trans_job_resp, public incoming_msg {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        trans_job_resp_in(incoming_msg * inc_msg)
                        : trans_job_resp(), m_inc_msg(inc_msg), m_data(NULL), m_iter(), m_sent_data(NULL) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_resp_in() {
                            //Delete the incoming message, it must always be present
                            delete m_inc_msg;
                            //Delete the sentence data if present
                            if (m_sent_data != NULL) {
                                delete m_sent_data;
                            }
                        }

                        /**
                         * Allows to iterate through the sentence data objects.
                         * This is meant to be done one time only
                         * @return a pointer to the sentence data object or NULL if we reached the end of the list
                         */
                        inline const trans_sent_data_in * next_send_data() {
                            if (m_data == NULL) {
                                //If the data was not yet retrieved, do it
                                m_data = &m_inc_msg->get_value(TARGET_DATA_FIELD_NAME);

                                //Perform a sanity check
                                ASSERT_SANITY_THROW(!m_data->is_array(), string("The value of '") +
                                        TARGET_DATA_FIELD_NAME + string("' is not an array!"));
                                
                                //Check if the vector size is above zero
                                if (!m_data->empty()) {
                                    //Initialize the iterator with the first value
                                    m_iter = m_data->cbegin();
                                    //Create the sentence data wrapper
                                    m_sent_data = new trans_sent_data_in(*m_iter);
                                    //Move to the next element
                                    ++m_iter;
                                }
                            } else {
                                //The data has been retrieved, so this is not the first iteration
                                if (m_iter == m_data->cend()) {
                                    //If we have no more elements or thre is not elements left
                                    if (m_sent_data != NULL) {
                                        //Delete the sentence data object if it is (still) present
                                        delete m_sent_data;
                                        m_sent_data = NULL;
                                    }
                                } else {
                                    //If there is more elements, set them into the sentence data
                                    m_sent_data->set_sent_data(*m_iter);
                                    //Move to the next element
                                    ++m_iter;
                                }
                            }

                            return m_sent_data;
                        }

                        /**
                         * Allows to get the job id
                         * @return the job id
                         */
                        inline const json & get_job_id() const {
                            return m_inc_msg->get_value(JOB_ID_FIELD_NAME);
                        }

                        /**
                         * Allows to get the translation task result code
                         * @return the translation task result code
                         */
                        inline status_code get_status_code() const {
                            //Get the status code value
                            int32_t code_value = m_inc_msg->get_value(STAT_CODE_FIELD_NAME);
                            //Create the status code class instance
                            return status_code(code_value);
                        }

                        /**
                         * Allows to get the translation task status message
                         * @return the translation task status message
                         */
                        inline const json & get_status_msg() const {
                            return m_inc_msg->get_value(STAT_MSG_FIELD_NAME);
                        }

                    private:
                        //Stores the pointer to the incoming message storing
                        //the response data, this pointer must not be NULL
                        incoming_msg * m_inc_msg;
                        //Stores the pointer to the translation data
                        const json * m_data;
                        //Stores the iterator
                        json::const_iterator m_iter;
                        //Stores the pointer to the sentence data
                        trans_sent_data_in * m_sent_data;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_RESP_IN_HPP */

