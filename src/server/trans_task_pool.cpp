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

#include <functional>

#include "server/trans_task_pool.hpp"
#include "server/trans_task_pool_worker.hpp"

using namespace std;
using namespace std::placeholders;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {

                trans_task_pool::trans_task_pool(const size_t num_threads)
                : m_stop(false) {
                    for (size_t i = 0; i < num_threads; ++i) {
                        m_workers.push_back(thread(trans_task_pool_worker(*this)));
                    }
                };

                trans_task_pool::~trans_task_pool() {
                    //Set the stopping flag
                    m_stop = true;
                    //Notify all the sleeping threads
                    m_condition.notify_all();
                    //Iterate through all the workers and wait until they exit
                    for (size_t i = 0; i < m_workers.size(); ++i) {
                        m_workers[i].join();
                    }
                };

                void trans_task_pool::notify_task_cancel(trans_task_ptr trans_task) {
                    unique_guard guard(m_queue_mutex);

                    LOG_DEBUG << "Request task  " << trans_task << " with id "
                            << trans_task->get_task_id() << " removal from the pool!" << END_LOG;

                    //ToDo: To improve performance we could try checking if the
                    //tasks is already running, and if not then search the queue.
                    //Or use other data structure for a more efficient task removal.
                    //This is for the future, in case the performance is affected.

                    //Check if the task is in the pool, if yes then remove it
                    for (tasks_queue_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                        if ((*it) == trans_task) {
                            m_tasks.erase(it);
                            LOG_DEBUG << "Task  " << trans_task << " with id " << trans_task->get_task_id()
                                    << " is found and erased" << END_LOG;
                            break;
                        }
                    }

                    LOG_DEBUG << "Task  " << trans_task << " with id " << trans_task->get_task_id()
                            << " removal from the pool is done!" << END_LOG;

                    //Note: If the task is already being run then it will be canceled by itself
                }

                void trans_task_pool::plan_new_task(trans_task_ptr trans_task) {
                    LOG_DEBUG << "Request adding a new task " << trans_task << " with id "
                            << trans_task->get_task_id() << " to the pool!" << END_LOG;

                    //Set the translation task with the method that should be called on the task cancel
                    trans_task->set_cancel_task_notifier(bind(&trans_task_pool::notify_task_cancel, this, _1));

                    //Add the task to the pool
                    {
                        unique_guard guard(m_queue_mutex);

                        //Add the translation task to the queue
                        m_tasks.push_back(trans_task);
                    }

                    //Wake up a thread to do a translation job
                    m_condition.notify_one();
                };
            }
        }
    }
}
