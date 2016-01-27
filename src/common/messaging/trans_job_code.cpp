/* 
 * File:   trans_job_code.cpp
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
 * Created on January 27, 2016, 10:42 AM
 */

#include "common/messaging/trans_job_code.hpp"

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

                    //Define the status strings
#define RESULT_UNKNOWN_STR "unknown"
#define RESULT_UNDEFINED_STR "undefined"
#define RESULT_OK_STR "good"
#define RESULT_ERROR_STR "error"
#define RESULT_CANCELED_STR "canceled"
#define RESULT_PARTIAL_STR "partial"

                    //Stores the status to string mappings
                    const char * const trans_job_code::m_code_str[values::size] = {
                        RESULT_UNDEFINED_STR,
                        RESULT_OK_STR,
                        RESULT_ERROR_STR,
                        RESULT_CANCELED_STR,
                        RESULT_PARTIAL_STR
                    };

                    /**
                     * Allows to get the job code string for reporting
                     * @return the job code string
                     */
                    const char * const trans_job_code::str() const {
                        if (m_code < values::size) {
                            return m_code_str[m_code];
                        } else {
                            return RESULT_UNKNOWN_STR;
                        }
                    }

                    /**
                     * The stream output operator for the given translation job code instance
                     * @param os the output stream
                     * @param code the code to be output
                     * @return the output stream
                     */
                    ostream& operator<<(ostream& os, const trans_job_code& code) {
                        os << code.str();
                        return os;
                    }
                }
            }
        }
    }
}
