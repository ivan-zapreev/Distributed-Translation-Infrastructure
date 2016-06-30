/* 
 * File:   trans_job_resp.hpp
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
 * Created on June 23, 2016, 3:36 PM
 */

#ifndef TRANS_JOB_RESP_HPP
#define TRANS_JOB_RESP_HPP

#include <common/messaging/response_msg.hpp>

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the base class for all the translation job response messages
                     */
                    class trans_job_resp : public response_msg {
                    public:
                        //The job id field name
                        static const char * JOB_ID_FIELD_NAME;
                        //The target data field name
                        static const char * TARGET_DATA_FIELD_NAME;

                        /**
                         * The basic constructor
                         */
                        trans_job_resp() : response_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_resp() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* TRANS_JOB_RESP_HPP */

