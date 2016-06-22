/* 
 * File:   trans_job_resp_data.hpp
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
 * Created on June 22, 2016, 11:02 AM
 */

#ifndef TRANS_JOB_RESP_DATA_HPP
#define	TRANS_JOB_RESP_DATA_HPP

#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/json_msg.hpp"
#include "common/messaging/trans_job_code.hpp"
#include "common/messaging/trans_job_id.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents the translation job
                     * response data to be sent to the client.
                     */
                    class trans_job_resp_data {
                    public:
                        //The job id field name
                        static const string JOB_ID_FIELD_NAME;
                        //The target data field name
                        static const string TARGET_DATA_FIELD_NAME;

                        /**
                         * The basic constructor.
                         */
                        trans_job_resp_data()
                        : m_msg(msg_type::MESSAGE_TRANS_JOB_RESP) {
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation status code. This
                         * constructor is to be used in case of error situations.
                         * @param job_id the client-issued id of the translation job 
                         * @param status_code the translation job result code
                         * @param status_msg the translation job status message
                         */
                        trans_job_resp_data(const job_id_type job_id,
                                const trans_job_code status_code, const string & status_msg)
                        : m_msg(msg_type::MESSAGE_TRANS_JOB_RESP) {
                            //Set the job id
                            set_job_id(job_id);
                            //Set the status
                            m_msg.set_status(status_code, status_msg);
                        }

                        /**
                         * Allows to set the job id
                         * @param job_id the job id
                         */
                        inline void set_job_id(const job_id_type job_id) {
                            m_msg.set_value(JOB_ID_FIELD_NAME, job_id);
                        }

                        /**
                         * Allows to set the status
                         * @param status_code the status code
                         * @param status_msg the status message
                         */
                        inline void set_status(const trans_job_code status_code,
                                const string & status_msg) {
                            m_msg.set_status(status_code, status_msg);
                        }

                    protected:
                        //Stores the json message
                        json_msg m_msg;
                    };
                }
            }
        }
    }
}

#endif	/* TRANS_JOB_RESP_DATA_HPP */

