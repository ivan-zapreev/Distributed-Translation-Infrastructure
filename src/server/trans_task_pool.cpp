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

using namespace std;
using namespace std::placeholders;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                trans_task_pool::trans_task_pool(const size_t num_threads)
                : m_tasks(), m_queue_mutex(), m_condition(), m_stop(false), m_threads(), m_workers() {
                    for (size_t i = 0; i < num_threads; ++i) {
                        //Add the new worker
                        m_workers.emplace_back(new trans_task_pool_worker(*this));
                        //Add the worker thread
                        m_threads.emplace_back(thread(&trans_task_pool_worker::operator(), m_workers.back()));
                    }
                };

                /**
                 * Allows to set the new number of worker threads
                 * @param new_num_threads the new number of worker threads
                 */
                void trans_task_pool::set_num_threads(const size_t new_num_threads) {
                    ASSERT_CONDITION_THROW((new_num_threads == 0),
                            string("The new number of threads is 0, must be > 0!"));

                    //Ge the current number of threads
                    const size_t curr_num_threads = m_threads.size();

                    LOG_DEBUG1 << "The old # threads: " << to_string(curr_num_threads)
                            << ", the new # threads: " << to_string(new_num_threads) << END_LOG;

                    if (new_num_threads > curr_num_threads) {
                        //We are to add more worker threads
                        for (size_t count = curr_num_threads; count < new_num_threads; ++count) {
                            //Add the new worker
                            m_workers.emplace_back(new trans_task_pool_worker(*this));
                            //Add the worker thread
                            m_threads.emplace_back(thread(&trans_task_pool_worker::operator(), m_workers.back()));
                        }
                    } else {
                        if (new_num_threads < curr_num_threads) {
                            //The number of workers to delete
                            const size_t num_to_delete = (curr_num_threads - new_num_threads);

                            LOG_WARNING << num_to_delete << " worker threads are about to be deleted, please wait!" << END_LOG;

                            //Delete the requested number of workers
                            size_t count = 0;
                            int32_t idx = 0;
                            while (count < num_to_delete) {
                                LOG_DEBUG1 << "Checking worker " << idx << "/" << m_workers.size() << END_LOG;

                                //If the worker is not busy, try to delete it
                                if (!m_workers[idx]->is_busy()) {
                                    LOG_DEBUG1 << "The worker " << idx << " is free!" << END_LOG;

                                    //Ask the worker to stop
                                    m_workers[idx]->stop();
                                    
                                    //Wake up all sleeping threads as the notify_one sends a wake 
                                    //up signal to a random thread but not the one we want to stop.
                                    m_condition.notify_all();
                                    
                                    //If the thread is joinable
                                    if (m_threads[idx].joinable()) {
                                        LOG_DEBUG1 << "Joining the thread: " << to_string(idx) << END_LOG;
                                        //Wait until the thread is finished
                                        m_threads[idx].join();
                                    }

                                    //Delete the worker
                                    LOG_DEBUG1 << "Deleting worker: " << to_string(idx) << END_LOG;
                                    delete m_workers[idx];

                                    //Erase the worker and its thread
                                    LOG_DEBUG1 << "Erasing the worker: " << to_string(idx) << " pointer" << END_LOG;
                                    m_workers.erase(m_workers.begin() + idx);
                                    LOG_DEBUG1 << "Erasing the worker's: " << to_string(idx) << "  thread" << END_LOG;
                                    m_threads.erase(m_threads.begin() + idx);

                                    //Increment the deleted workers count
                                    ++count;
                                }

                                //Move on through the workers
                                idx = (idx + 1) % m_workers.size();
                            }
                        }
                    }
                }

                trans_task_pool::~trans_task_pool() {
                    //Set the stopping flag
                    m_stop = true;
                    //Notify all the sleeping threads
                    m_condition.notify_all();
                    //Iterate through all the workers and wait until they exit
                    for (size_t i = 0; i < m_threads.size(); ++i) {
                        //Wait until the thread is finished
                        m_threads[i].join();
                        //Delete the worker
                        delete m_workers[i];
                    }
                };

                /**
                 * \todo {To improve performance we could try checking if the
                 * tasks is already running, and if not then search the queue.
                 * Or use other data structure for a more efficient task removal.
                 * This is for the future, in case the performance is affected.}
                 */
                void trans_task_pool::notify_task_cancel(trans_task_ptr trans_task) {
                    unique_guard guard(m_queue_mutex);

                    LOG_DEBUG << "Request task  " << *trans_task << " removal from the pool!" << END_LOG;

                    //Check if the task is in the pool, if yes then remove it
                    for (tasks_queue_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                        if ((*it) == trans_task) {
                            m_tasks.erase(it);
                            LOG_DEBUG << "Task  " << *trans_task << " is found and erased" << END_LOG;
                            break;
                        }
                    }

                    LOG_DEBUG << "Task  " << *trans_task << " removal from the pool is done!" << END_LOG;

                    //Note: If the task is already being run then it will be canceled by itself
                }

                void trans_task_pool::plan_new_task(trans_task_ptr trans_task) {
                    LOG_DEBUG << "Request adding a new task " << *trans_task << " to the pool!" << END_LOG;

                    //Set the translation task with the method that should be called on the task cancel
                    trans_task->set_cancel_task_notifier(bind(&trans_task_pool::notify_task_cancel, this, _1));

                    //Add the task to the pool
                    {
                        unique_guard guard(m_queue_mutex);

                        LOG_DEBUG << "Pushing the task " << *trans_task << " to the list of translation tasks." << END_LOG;

                        //Add the translation task to the queue
                        m_tasks.push_back(trans_task);
                    }

                    LOG_DEBUG << "Notifying threads that there is a translation task present!" << END_LOG;

                    //Wake up a thread to do a translation job
                    m_condition.notify_one();

                    LOG_DEBUG << "Done planning a new task." << END_LOG;
                };
            }
        }
    }
}
