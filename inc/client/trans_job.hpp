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

#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_response.hpp"

using namespace uva::smt::decoding::common::messaging;

#ifndef TRANS_JOB_HPP
#define TRANS_JOB_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

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
                        STATUS_REQ_RECEIVED = STATUS_REQ_SENT_FAIL + 1, //The translation response received
                        size = STATUS_REQ_RECEIVED + 1
                    };

                    /**
                     * The basic constructor that does default-initialization of the structure fields
                     */
                    trans_job()
                    : m_num_sent(0), m_request(NULL), m_response(NULL),
                    m_failed_to_send(status::STATUS_INITIAL) {
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

                    //The number of sentences to be translated
                    uint32_t m_num_sent;
                    //The pointer to the job request
                    trans_job_request_ptr m_request;
                    //The pointer to the job response
                    trans_job_response_ptr m_response;
                    //Stores the flag indicating whether the job was failed to send
                    status m_failed_to_send;
                };

                //Define the pointer type for the translation job data
                typedef trans_job * trans_job_data_ptr;
            }
        }
    }
}

#endif /* TRANS_JOB_HPP */

