/* 
 * File:   processor_manager.hpp
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
 * Created on July 25, 2016, 11:42 AM
 */

#ifndef PROCESSOR_MANAGER_HPP
#define PROCESSOR_MANAGER_HPP


#include <unordered_map>
#include <set>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/task_pool.hpp"

#include "common/messaging/session_manager.hpp"
#include "common/messaging/session_job_pool_base.hpp"

#include "processor/processor_parameters.hpp"
#include "processor/processor_job.hpp"

#include "processor/messaging/pre_proc_req_in.hpp"
#include "processor/messaging/post_proc_req_in.hpp"

using namespace std;
using namespace std::placeholders;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                /**
                 * This is the processor manager class:
                 * Responsibilities:
                 *      Keep track of client sessions
                 *      Keep track of client job requests
                 *      Cancel job requests for disconnected clients
                 *      Report error job in case could not dispatch
                 *      Send the processor response for a job
                 *      Maintain the processor jobs queue
                 */
                class processor_manager : public session_manager, private session_job_pool_base<processor_job> {
                public:
                    
                    /**
                     * The basic constructor
                     * @param num_threads the number of threads to handle the processor jobs
                     */
                    processor_manager(const processor_parameters & params)
                    : session_manager(), session_job_pool_base(
                    bind(&processor_manager::notify_job_done, this, _1)),
                    m_params(params), m_proc_pool(params.m_num_threads) {
                    }

                    /**
                     * Reports the run-time information
                     */
                    inline void report_run_time_info() {
                        //Report the super class info first
                        session_job_pool_base::report_run_time_info();

                        //Report data from the task pools
                        m_proc_pool.report_run_time_info("Jobs pool");
                    }

                    /**
                     * Allows to set a new number of pool threads
                     * @param num_threads the new number of threads
                     */
                    inline void set_num_threads(const int32_t num_threads) {
                        m_proc_pool.set_num_threads(num_threads);
                    }

                    /**
                     * Allows to stop the manager
                     */
                    inline void stop() {
                        session_job_pool_base<processor_job>::stop();
                    }

                    /**
                     * Allows to schedule the incoming pre-processor request
                     * @param hdl the connection handler to identify the session object.
                     * @param msg a pointer to the request data, not NULL
                     */
                    inline void pre_process(websocketpp::connection_hdl hdl, pre_proc_req_in * msg) {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to schedule the incoming post-processor request
                     * @param hdl the connection handler to identify the session object.
                     * @param msg a pointer to the request data, not NULL
                     */
                    inline void post_process(websocketpp::connection_hdl hdl, post_proc_req_in * msg) {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                protected:
                    /**
                     * Will be called once a new job is scheduled. Here we need to perform
                     * the needed actions with the job. I.e. add it into the incoming tasks pool.
                     * @see session_job_pool_base
                     */
                    virtual void schedule_new_job(proc_job_ptr proc_job) {
                        //Plan the job in to the incoming tasks pool and move on.
                        m_proc_pool.plan_new_task(proc_job);
                    }

                    /**
                     * Will be called when a session with the given id is closed
                     * @see session_manager
                     */
                    virtual void session_is_closed(session_id_type session_id) {
                        //Cancel all the jobs from the given session
                        this->cancel_jobs(session_id);
                    }
                    
                    /**
                     * This function will be called once the processor job is fully
                     * done an it is about to be destroyed. This function can be
                     * used to remove the job from the internal mappings.
                     * @param proc_job the processor job that is fully done
                     */
                    inline void notify_job_done(proc_job_ptr proc_job) {
                        LOG_DEBUG << "Finishing off processed job " << *proc_job << END_LOG;

                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                private:
                    //Stores the reference to the set of processor parameters
                    const processor_parameters & m_params;
                    //Stores the tasks pool
                    task_pool<processor_job> m_proc_pool;
                    
                };
            }
        }
    }
}

#endif /* PROCESSOR_MANAGER_HPP */

