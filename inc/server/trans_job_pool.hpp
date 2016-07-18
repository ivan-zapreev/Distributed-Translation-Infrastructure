/* 
 * File:   trans_job_pool.hpp
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
 * Created on January 20, 2016, 3:01 PM
 */

#include <unordered_map>
#include <vector>

#include "common/messaging/session_job_pool_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"

#include "common/utils/threads/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/task_pool.hpp"

#include "server/trans_task_id.hpp"
#include "server/trans_job.hpp"

using namespace std;
using namespace std::placeholders;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::smt::bpbd::common::messaging;

#ifndef TRANS_JOB_POOL_HPP
#define TRANS_JOB_POOL_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This class is used to schedule the translation jobs.
                 * Each translation job consists of a number of sentences to translate.
                 * Each sentence will be translated in its own thread with its own decoder instance.
                 * The job of this class is to split the translation job into a number
                 * of translation tasks and schedule them. This class is synchronized
                 * and has its own thread to schedule the translation tasks.
                 */
                class trans_job_pool : public session_job_pool_base<trans_job> {
                public:

                    /**
                     * The basic constructor,  starts the finished jobs processing thread.
                     * @param num_threads the number of translation threads to run
                     * @param notify_job_done_func the setter functional to be set
                     */
                    trans_job_pool(const size_t num_threads, done_job_notifier notify_job_done_func)
                    : session_job_pool_base(notify_job_done_func), m_tasks_pool(num_threads) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_job_pool() {
                        //Nothing to be done here
                    }

                    /**
                     * Allows to set the new number of worker threads.
                     * This operation should be safe as the new threads
                     * are just added to the list and the deleted ones
                     * are let to finish their translation task execution. 
                     * @param num_threads the new number of worker threads
                     */
                    inline void set_num_threads(const size_t num_threads) {
                        m_tasks_pool.set_num_threads(num_threads);
                    }

                    /**
                     * Allows to report the runtime information.
                     */
                    virtual void report_run_time_info() {
                        //Report the super class info first
                        session_job_pool_base::report_run_time_info();

                        //Report data from the tasks pool
                        m_tasks_pool.report_run_time_info("Translation tasks pool");
                    }

                protected:

                    /**
                     * @see session_job_pool_base
                     */
                    virtual void process_new_job(trans_job_ptr trans_job) {
                        //Add the job tasks to the tasks' pool
                        const trans_job::tasks_list_type& tasks = trans_job->get_tasks();
                        for (trans_job::tasks_const_iter_type it = tasks.begin(); it != tasks.end(); ++it) {
                            m_tasks_pool.plan_new_task(*it);
                        }
                    }

                private:
                    //Stores the tasks pool
                    task_pool<trans_task> m_tasks_pool;
                };
            }
        }
    }
}

#endif /* TRANS_JOB_POOL_HPP */

