/* 
 * File:   trans_job_response.hpp
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
 * Created on January 18, 2016, 5:02 PM
 */

#ifndef TRANSLATION_JOB_REPLY_HPP
#define TRANSLATION_JOB_REPLY_HPP

#include <string>
#include <sstream>
#include <iostream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_code.hpp"
#include "common/messaging/trans_job_resp_data.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    //Do a forward definition of the class
                    class trans_job_response;

                    //Define the pointer type for the job response
                    typedef trans_job_response * trans_job_response_ptr;

                    /**
                     * This class represents the translation reply message, which
                     * is a translation result for a translation job. This result
                     * can be a text in the target language or it can be an error.
                     */
                    class trans_job_response : public trans_job_resp_data {
                    public:

                        /**
                         * The basic no-argument constructor.
                         * It default-initializes the class with undefined values.
                         */
                        trans_job_response()
                        : trans_job_resp_data() {
                        }

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation result code. This
                         * constructor is to be used in case of error situations.
                         * @param job_id the client-issued id of the translation job 
                         * @param status_code the translation job result code
                         * @param status_msg the translation job status message
                         */
                        trans_job_response(const job_id_type job_id,
                                const trans_job_code status_code,
                                const string & status_msg)
                        : trans_job_resp_data(job_id, status_code, status_msg) {
                        }

                        /**
                         * Allows to de-serialize the job reply from a string
                         * @param data the string representation of the translation job reply
                         */
                        void de_serialize(const string & data) {
                            LOG_DEBUG1 << "De-serializing response message: '" << data << "'" << END_LOG;
                            m_msg.de_serialize(data);
                        }

                        /**
                         * Allows to serialize the translation job response into a string
                         * @return the string representation of the translation job response
                         */
                        inline const string serialize() const {
                            const string result = m_msg.serialize();
                            LOG_DEBUG1 << "Serializing response message: '" << result << "'" << END_LOG;
                            return result;
                        }

                        /**
                         * Allows to retrieve the value of the status code
                         * @return the status code
                         */
                        inline trans_job_code get_status_code() {
                            return m_msg.get_status_code();
                        }

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline job_id_type get_job_id() const {
                            return m_msg.get_value<job_id_type>(JOB_ID_FIELD_NAME);
                        }

                        /**
                         * Allows to get the translation job text. This is either
                         * the text translated into the target language or the error
                         * message for the case of failed translation job request.
                         * @return the translation job text
                         */
                        inline const string get_target_text() const {
                            //ToDo: Implement
                            THROW_NOT_IMPLEMENTED();
                        }
                    };
                }
            }
        }
    }
}

#endif /* TRANSLATION_JOB_REPLY_HPP */

