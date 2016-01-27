/* 
 * File:   trans_job_status.hpp
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
 * Created on January 27, 2016, 9:03 AM
 */

#include "common/utils/logging/Logger.hpp"

using namespace uva::utils::logging;

#ifndef TRANS_JOB_STATUS_HPP
#define TRANS_JOB_STATUS_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {
                namespace status {

                    //Define the status strings
#define STATUS_UNKNOWN_STR "unknown"
#define STATUS_UNDEFINED_STR "undefined"
#define STATUS_REQ_INITIALIZED_STR "not-sent"
#define STATUS_REQ_SENT_GOOD_STR "not-replied"
#define STATUS_REQ_SENT_FAIL_STR "send-failed"
#define STATUS_RES_RECEIVED_STR "replied"

                    /**
                     * Stores the possible status values of the client-side translation job
                     */
                    enum trans_job_status {
                        STATUS_UNDEFINED = 0, //The job has been created but not initialized, i.e. undefined
                        STATUS_REQ_INITIALIZED = STATUS_UNDEFINED + 1, //Initialized with the translation request
                        STATUS_REQ_SENT_GOOD = STATUS_REQ_INITIALIZED + 1, //The translation request is sent
                        STATUS_REQ_SENT_FAIL = STATUS_REQ_SENT_GOOD + 1, //The translation request failed to sent
                        STATUS_RES_RECEIVED = STATUS_REQ_SENT_FAIL + 1, //The translation response was received
                        size = STATUS_RES_RECEIVED + 1
                    };

                    //Stores the status to string mappings
                    static const char * const m_status_str[trans_job_status::size] = {
                        STATUS_UNDEFINED_STR,
                        STATUS_REQ_INITIALIZED_STR,
                        STATUS_REQ_SENT_GOOD_STR,
                        STATUS_REQ_SENT_FAIL_STR,
                        STATUS_RES_RECEIVED_STR
                    };

                    /**
                     * Allows to get the job status string for reporting
                     * @return the job status string
                     */
                    static const char * const get_status_str(trans_job_status status) {
                        if (status < trans_job_status::size) {
                            return m_status_str[status];
                        } else {
                            LOG_ERROR << "The job status has not string: " << status << END_LOG;
                            return STATUS_UNKNOWN_STR;
                        }
                    }

                }
            }
        }
    }
}

#endif /* TRANS_JOB_STATUS_HPP */

