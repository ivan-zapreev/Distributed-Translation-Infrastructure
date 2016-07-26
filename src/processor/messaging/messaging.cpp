/* 
 * File:   messaging.cpp
 * Author: zapreevis
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
 * Created on July 26, 2016, 1:29 PM
 */

#include "processor/messaging/pre_proc_req_in.hpp"
#include "processor/messaging/post_proc_req_in.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    const char * JOB_ID_FIELD_NAME = "job_id";
                    const char * TASK_ID_FIELD_NAME = "task_id";
                    const char * NUM_TASKS_FIELD_NAME = "num_tasks";
                    const char * SOURCE_LANG_FIELD_NAME = "lang";
                    const char * SOURCE_TEXT_FIELD_NAME = "text";
                    const char * DETECT_LANGUAGE_VALUE = "<auto>";
                }
            }
        }
    }
}