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

#include "trans_job_status.hpp"
#include "common/utils/Exceptions.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_response.hpp"

using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;
using namespace uva::smt::decoding::client::status;

#ifndef TRANS_JOB_HPP
#define TRANS_JOB_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                //Do the forward definition of the class
                struct trans_job;

                //Define the pointer type for the translation job data
                typedef trans_job * trans_job_ptr;

                /**
                 * This structure is used for storing the translation job data
                 */
                struct trans_job {

                    /**
                     * The basic constructor that does default-initialization of the structure fields
                     */
                    trans_job()
                    : m_num_sentences(0), m_request(NULL), m_response(NULL),
                    m_status(trans_job_status::STATUS_UNDEFINED) {
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
                    uint32_t m_num_sentences;
                    //The pointer to the job request
                    trans_job_request_ptr m_request;
                    //The pointer to the job response
                    trans_job_response_ptr m_response;
                    //Stores the flag indicating whether the job was failed to send
                    trans_job_status m_status;
                };

            }
        }
    }
}

#endif /* TRANS_JOB_HPP */

