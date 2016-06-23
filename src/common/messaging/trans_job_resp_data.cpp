/* 
 * File:   trans_job_resp_data.cpp
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
 * Created on June 22, 2016, 15:57 PM
 */

#include "common/messaging/trans_job_resp_data.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    const string trans_job_resp_data::JOB_ID_FIELD_NAME = "job_id";
                    const string trans_job_resp_data::TARGET_DATA_FIELD_NAME = "target_data";
                    const string trans_job_resp_data::TRANS_TEXT_FIELD_NAME = "trans_text";
                    const string trans_job_resp_data::STACK_LOAD_FIELD_NAME = "stack_load";

                }
            }
        }
    }
}
