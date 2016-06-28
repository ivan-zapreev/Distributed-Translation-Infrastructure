/* 
 * File:   trans_job_resp_out.hpp
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

#ifndef TRANS_JOB_RESP_OUT_HPP
#define TRANS_JOB_RESP_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/trans_job_resp.hpp"

#include "server/messaging/trans_sent_data_out.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    /**
                     * This class represents a translation job response message to be sent to the client
                     */
                    class trans_job_resp_out : public outgoing_msg, public trans_job_resp {
                    public:

                        /**
                         * The basic class constructor
                         */
                        trans_job_resp_out()
                        : outgoing_msg(msg_type::MESSAGE_TRANS_JOB_RESP), trans_job_resp(),
                        m_sent_data(NULL), m_empty_sent_data() {
                            //Nothing to be done here
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation result code. This
                         * constructor is to be used in case of error situations.
                         * @param job_id the client-issued id of the translation job 
                         * @param status_code the translation job result code
                         * @param status_msg the translation job status message
                         */
                        trans_job_resp_out(const job_id_type job_id,
                                const status_code code,
                                const string & msg)
                        : outgoing_msg(msg_type::MESSAGE_TRANS_JOB_RESP), trans_job_resp(),
                        m_sent_data(NULL), m_empty_sent_data() {
                            //Set the values using the setter methods
                            set_job_id(job_id);
                            set_status(code, msg);
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~trans_job_resp_out() {
                            LOG_DEBUG2 << "Deleting the translation job response" << END_LOG;
                            //Delete the sentence data wrapper if any
                            if (m_sent_data != NULL) {
                                LOG_DEBUG2 << "Deleting the sentence response data wrapper" << END_LOG;
                                delete m_sent_data;
                            }
                        }

                        /**
                         * Allows to set the message status: code and message
                         * @param code the status code
                         * @param msg the status message
                         */
                        inline void set_status(const status_code & code, const string & msg) {
                            m_json[STAT_CODE_FIELD_NAME] = code.val();
                            m_json[STAT_MSG_FIELD_NAME] = msg;
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline void set_job_id(const job_id_type job_id) {
                            m_json[JOB_ID_FIELD_NAME] = job_id;
                        }

                        /**
                         * Allows to add a new sentence data object into the list and
                         * return it in a wrapped around translation sentence data object.
                         * This method always return the same object, it only changes the
                         * object wrapped inside it.
                         * @return the same sentence data wrapper wrapping around different sentence data objects.
                         */
                        inline trans_sent_data_out & add_new_sent_data() {
                            //Get the target data array
                            json & data = m_json[TARGET_DATA_FIELD_NAME];

                            //Add a new JSON sentence data entry
                            data.push_back(m_empty_sent_data);
                            json & sent_data = data.at(data.size() - 1);

                            //If the sentence data is NULL create a new one
                            if (m_sent_data != NULL) {
                                m_sent_data->set_sent_data(sent_data);
                            } else {
                                ///Otherwise set the sentence data to the old one
                                m_sent_data = new trans_sent_data_out(sent_data);
                            }

                            //Return the reference to the data
                            return *m_sent_data;
                        }

                    private:
                        //Stores the pointer to the sentence data
                        trans_sent_data_out * m_sent_data;

                        //Stores an empty json object to be used for a sentence data
                        const json m_empty_sent_data;
                    };
                }
            }
        }
    }
}

#endif /* TRANS_JOB_RESP_OUT_HPP */

