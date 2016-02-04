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

using namespace std;
using namespace uva::utils::threads;

namespace uva {
    namespace smt {
        namespace translation {
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

                    /**
                     * This is a basic constructor accepting the number of threads parameter.
                     * @param num_threads the number of threads to be run by this task pool.
                     */
                    trans_task_pool(const size_t num_threads);

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
                    threads_list_type m_workers;
                };

            }
        }
    }
}


#endif /* TRANS_TASK_POOL_HPP */

