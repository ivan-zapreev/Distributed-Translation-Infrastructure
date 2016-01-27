/* 
 * File:   trans_job.hpp
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
 * Created on January 26, 2016, 5:24 PM
 */

#include "common/utils/Exceptions.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_response.hpp"

using namespace uva::smt::decoding::common::messaging;
using namespace uva::utils::exceptions;

#ifndef TRANS_JOB_HPP
#define TRANS_JOB_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                //Define the status strings
#define STATUS_UNKNOWN_STR "unknown"
#define STATUS_INITIAL_STR "not-set"
#define STATUS_REQ_INITIALIZED_STR "not-sent"
#define STATUS_REQ_SENT_GOOD_STR "not-replied"
#define STATUS_REQ_SENT_FAIL_STR "send-failed"
#define STATUS_RES_RECEIVED_STR "replied"

                //Do the forward definition of the class
                struct trans_job;

                //Define the pointer type for the translation job data
                typedef trans_job * trans_job_ptr;

                /**
                 * This structure is used for storing the translation job data
                 */
                struct trans_job {
                    //Stores the possible status of the translation job data

                    enum status {
                        STATUS_INITIAL = 0, //The job has been created but not initialized
                        STATUS_REQ_INITIALIZED = STATUS_INITIAL + 1, //Initialized with the translation request
                        STATUS_REQ_SENT_GOOD = STATUS_REQ_INITIALIZED + 1, //The translation request is sent
                        STATUS_REQ_SENT_FAIL = STATUS_REQ_SENT_GOOD + 1, //The translation request failed to sent
                        STATUS_RES_RECEIVED = STATUS_REQ_SENT_FAIL + 1, //The translation response was received
                        size = STATUS_RES_RECEIVED + 1
                    };

                    //Stores the status to string mappings
                    static const char * const m_status_str[status::size];

                    /**
                     * The basic constructor that does default-initialization of the structure fields
                     */
                    trans_job()
                    : m_num_sentences(0), m_request(NULL), m_response(NULL),
                    m_status(status::STATUS_INITIAL) {
                    }

                    /**
                     * The basic destructor that allows to delete the dynamically
                     * allocated data pointed by the structure fields
                     */
                    virtual ~trans_job() {
                        if (m_request != NULL) {
                            delete m_request;
                        }
                        if (m_response != NULL) {
                            delete m_response;
                        }
                    }

                    /**
                     * Allows to get the job status string for reporting
                     * @return 
                     */
                    const char * const get_status_str() {
                        if (m_status < status::size) {
                            return m_status_str[m_status];
                        } else {
                            LOG_ERROR << "The job status has not string: " << m_status << END_LOG;
                            return STATUS_UNKNOWN_STR;
                        }
                    }

                    //The number of sentences to be translated
                    uint32_t m_num_sentences;
                    //The pointer to the job request
                    trans_job_request_ptr m_request;
                    //The pointer to the job response
                    trans_job_response_ptr m_response;
                    //Stores the flag indicating whether the job was failed to send
                    status m_status;
                };

                const char * const trans_job::m_status_str[status::size] = {
                    STATUS_INITIAL_STR,
                    STATUS_REQ_INITIALIZED_STR,
                    STATUS_REQ_SENT_GOOD_STR,
                    STATUS_REQ_SENT_FAIL_STR,
                    STATUS_RES_RECEIVED_STR
                };

            }
        }
    }
}

#endif /* TRANS_JOB_HPP */

