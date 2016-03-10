/* 
 * File:   task_pool_worker.hpp
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
 * Created on January 25, 2016, 1:46 PM
 */

#include "server/trans_task_pool_worker.hpp"
#include "server/trans_task_pool.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                void trans_task_pool_worker::operator()() {
                    trans_task_ptr task;

                    //Run the thread as long as it is not stopped
                    while (!m_pool.m_stop && m_worker_is_on) {
                        //Lock the critical waiting section
                        {
                            unique_guard guard(m_pool.m_queue_mutex);

                            //Wait for the new task is scheduled or we need to stop
                            while (!m_pool.m_stop && m_worker_is_on && m_pool.m_tasks.empty()) {
                                m_pool.m_condition.wait(guard);
                            }

                            //Check on the situation we are in
                            if (m_pool.m_stop || !m_worker_is_on) {
                                //If we need to stop then return - finish the thread
                                return;
                            } else {
                                //There is a task to be processed, retrieve it
                                task = m_pool.m_tasks.front();
                                m_pool.m_tasks.pop_front();
                            }
                        }

                        //Execute the newly obtained task
                        task->translate();
                    }
                }
            }
        }
    }
}
