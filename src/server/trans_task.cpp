/* 
 * File:   trans_task.hpp
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
 * Created on May 25, 2016, 9:14 AM
 */

#include "server/trans_task.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                id_manager<task_id_type> trans_task::m_id_mgr(task_id::MINIMUM_TASK_ID);

                /**
                 * Allows to log the translation task into an output stream
                 * @param stream the output stream
                 * @param task the task to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const trans_task & task) {
                    return stream << to_string(task.m_task_id);
                }
            }
        }
    }
}
