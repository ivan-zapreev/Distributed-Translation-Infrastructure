/* 
 * File:   trans_job_status.cpp
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
 * Created on January 27, 2016, 10:50 AM
 */

#include "client/trans_job_status.hpp"

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                //Define the status strings
#define STATUS_UNKNOWN_STR "unknown"
#define STATUS_UNDEFINED_STR "undefined"
#define STATUS_REQ_INITIALIZED_STR "not-sent"
#define STATUS_REQ_SENT_GOOD_STR "not-replied"
#define STATUS_REQ_SENT_FAIL_STR "send-failed"
#define STATUS_RES_RECEIVED_STR "replied"

                const char * const trans_job_status::m_status_str[trans_job_status::size] = {
                    STATUS_UNDEFINED_STR,
                    STATUS_REQ_INITIALIZED_STR,
                    STATUS_REQ_SENT_GOOD_STR,
                    STATUS_REQ_SENT_FAIL_STR,
                    STATUS_RES_RECEIVED_STR
                };

                const char * const trans_job_status::str() const {
                    if (m_status < values::size) {
                        return m_status_str[m_status];
                    } else {
                        return STATUS_UNKNOWN_STR;
                    }
                }

                ostream& operator<<(ostream& os, const trans_job_status& status) {
                    os << status.str();
                    return os;
                }

            }
        }
    }
}