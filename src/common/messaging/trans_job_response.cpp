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

#include "common/messaging/trans_job_response.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    constexpr char trans_job_response::HEADER_DELIMITER;
                    constexpr char trans_job_response::NEW_LINE_HEADER_ENDING;
                    
                    const string trans_job_response::TRANS_JOB_RESPONSE_PREFIX = string("TRAN_JOB_RESP") + HEADER_DELIMITER;
                }
            }
        }
    }
}