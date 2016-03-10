/* 
 * File:   trans_task_pool.hpp
 * Author: zapreevis
 *
 * Created on January 21, 2016, 5:00 PM
 */

#ifndef TRANS_TASK_POOL_HPP
#define TRANS_TASK_POOL_HPP

#include <vector>
#include <deque>

#include "trans_task.hpp"
#include "common/utils/threads.hpp"

#include "server/trans_task_pool_worker.hpp"

using namespace std;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This class represents a translation tasks pool which is in essence a thread pool.
                 * One should add the translation tasks into this class and using the pre-configured
                 * number of threads it will execute these tasks one by one. This class is thread safe. 
                 */
                class trans_task_pool {
                public:

                    //Define the tasks queue type and its iterator
                    typedef deque<trans_task_ptr> tasks_queue_type;
                    typedef tasks_queue_type::iterator tasks_queue_iter_type;

                    //Define the thread list type
                    typedef vector<thread> threads_list_type;
                    //Define the workers list type
                    typedef vector<trans_task_pool_worker *> workers_list_type;
                    /**
                     * This is a basic constructor accepting the number of threads parameter.
                     * @param num_threads the number of threads to be run by this task pool.
                     */
                    trans_task_pool(const size_t num_threads);

                    /**
                     * Allows to set the new number of worker threads.
                     * This operation should be safe as the new threads
                     * are just added to the list and the deleted ones
                     * are let to finish their translation task execution. 
                     * @param num_threads the new number of worker threads
                     */
                    void set_num_threads(const size_t num_threads);

                    /**
                     * Allows to report the runtime information.
                     */
                    void report_run_time_info() {
                        //Add the task to the pool
                        {
                            unique_guard guard(m_queue_mutex);

                            //Count the active workers
                            size_t count = 0;
                            for (size_t idx = 0; idx < m_workers.size(); ++idx) {
                                if (m_workers[idx]->is_busy()) {
                                    ++count;
                                }
                            }

                            LOG_USAGE << "#pending tasks: " << m_tasks.size()
                                    << ", #active threads: " << count
                                    << "/" << m_threads.size() << END_LOG;
                        }
                    }

                    /**
                     * The class destructor
                     */
                    virtual ~trans_task_pool();

                    /**
                     * This method allows to plan a new translation task
                     * @param trans_task the translation task to plan
                     */
                    void plan_new_task(trans_task_ptr trans_task);

                protected:
                    //The worker class is a friend of this one, for simplicity
                    friend class trans_task_pool_worker;

                    //This is the queue of scheduled tasks
                    tasks_queue_type m_tasks;

                    //Stores the synchronization primitive instances
                    mutex m_queue_mutex;
                    condition_variable m_condition;

                    //Stores the stopping flag
                    atomic<bool> m_stop;

                    /**
                     * The method that will be called in case a task is canceled
                     * @param trans_task the task that is being canceled
                     */
                    void notify_task_cancel(trans_task_ptr trans_task);
                private:

                    //Stores the worker threads
                    threads_list_type m_threads;

                    //Stores the workers
                    workers_list_type m_workers;
                };

            }
        }
    }
}


#endif /* TRANS_TASK_POOL_HPP */

