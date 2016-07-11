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

#ifndef TASK_POOL_WORKER_HPP
#define TASK_POOL_WORKER_HPP

#include "common/utils/threads/threads.hpp"

using namespace std;
using namespace uva::utils::threads;

namespace uva {
    namespace utils {
        namespace threads {

            //Do a forward declaration of the task pool
            template<typename pool_task>
            class task_pool;

            /**
             * This class represents a translation tasks pool worker. This is
             * class is to be used around the actual translation task inside
             * the translation tasks pool. We need this class as a synchronization
             * layer for the thread pool, as each of instances of this class will
             * be run by a thread.
             */
            template<typename pool_task>
            class task_pool_worker {
            public:
                //Typedef the pointer to the pool task
                typedef pool_task * pool_task_ptr;

                /**
                 * This is a basic constructor that needs the thread pool reference as an argument.
                 * @param pool the task pool reference
                 */
                task_pool_worker(task_pool<pool_task> & pool)
                : m_is_on(true), m_is_busy(false), m_pool(pool) {
                }

                /**
                 * The basic destructor
                 */
                virtual ~task_pool_worker() {
                    //Nothing to be done, the thread is destroyed, no resources to free
                }

                /**
                 * Forces the given worker to stop
                 */
                inline void stop() {
                    m_is_on = false;
                }

                /**
                 * Returns the reference to the worker's is_busy flag
                 * @return the worker's is busy flag reference to see if the worker is busy or not.
                 */
                inline atomic<bool> & is_busy() {
                    return m_is_busy;
                }

                /**
                 * This operator will be called to run the thread, its implementation
                 * will run the tasks scheduled in the thread pool.
                 */
                inline void operator()() {
                    pool_task_ptr task;

                    //Run the thread as long as it is not stopped
                    while (!m_pool.m_stop && m_is_on) {
                        //Lock the critical waiting section
                        {
                            unique_guard guard(m_pool.m_queue_mutex);

                            //Wait for the new task is scheduled or we need to stop
                            while (!m_pool.m_stop && m_is_on && m_pool.m_tasks.empty()) {
                                m_pool.m_condition.wait(guard);
                            }

                            //Check on the situation we are in
                            if (m_pool.m_stop || !m_is_on) {
                                //If we need to stop then return - finish the thread
                                return;
                            } else {
                                //There is a task to be processed, retrieve it
                                task = m_pool.m_tasks.front();
                                m_pool.m_tasks.pop_front();
                            }
                        }

                        //Execute the newly obtained task
                        m_is_busy = true;
                        task->execute();
                        m_is_busy = false;
                    }
                }

            protected:
            private:
                //Stores the flag indicating if this worker is on duty or not.
                atomic<bool> m_is_on;
                //Stores the flag indicating if this worker is busy working or not.
                atomic<bool> m_is_busy;
                //Keeps the reference to the pool
                task_pool<pool_task> & m_pool;
            };
        }
    }
}

#endif /* TASK_POOL_WORKER_HPP */

