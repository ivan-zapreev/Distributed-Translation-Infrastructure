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
#include "common/utils/threads/threads.hpp"

#include "common/messaging/session_manager.hpp"
#include "common/messaging/session_job_pool_base.hpp"

#include "processor/processor_parameters.hpp"
#include "processor/processor_job.hpp"
#include "processor/pre_proc_job.hpp"
#include "processor/post_proc_job.hpp"

#include "processor/messaging/proc_req_in.hpp"

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
                     * The session entry structure to store the access mutex
                     * to the jobs map and the session jobs map itself.
                     */
                    struct session_data_struct {
                        //The map storing the mapping from the job id to the job object
                        unordered_map<string, proc_job_ptr> m_jobs;

                        /**
                         * The basic destcutor that shall remove all the jobs
                         */
                        virtual ~session_data_struct() {
                            //Just iterate through the map and delete all the jobs
                            //Note that there is no need to remove the jobs from mappings
                            for (auto iter = m_jobs.begin(); iter != m_jobs.end(); ++iter) {
                                delete iter->second;
                            }
                        }
                    };

                    //Typedef the structure for storing the session data
                    typedef struct session_data_struct session_data;

                    //Typedef the data structure for storing the mapping
                    //between the session id and the session data.
                    typedef unordered_map<session_id_type, session_data> sessions_map;

                    /**
                     * The basic constructor
                     * @param num_threads the number of threads to handle the processor jobs
                     */
                    processor_manager(const processor_parameters & params)
                    : session_manager(), session_job_pool_base(
                    bind(&processor_manager::notify_job_done, this, _1)),
                    m_is_stopping(false), m_params(params), m_proc_pool(params.m_num_threads),
                    m_resp_send_func(bind(&processor_manager::send_response, this, _1, _2)) {
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
                        recursive_guard guard(m_sessions_lock);

                        //Set the stopping flag
                        m_is_stopping = true;

                        //Go through all the registered sessions and cancel the incomplete jobs
                        for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter) {
                            cancel_incomp_jobs(iter->first);
                        }

                        //Stop the session job pool
                        session_job_pool_base<processor_job>::stop();
                    }

                    /**
                     * Allows to schedule the incoming pre-processor request
                     * @param hdl the connection handler to identify the session object.
                     * @param msg a pointer to the request data, not NULL
                     */
                    inline void pre_process(websocketpp::connection_hdl hdl, proc_req_in * msg) {
                        this->template process<pre_proc_job>(hdl, msg, m_params.m_pre_script_config);
                    }

                    /**
                     * Allows to schedule the incoming post-processor request
                     * @param hdl the connection handler to identify the session object.
                     * @param msg a pointer to the request data, not NULL
                     */
                    inline void post_process(websocketpp::connection_hdl hdl, proc_req_in * msg) {
                        this->template process<post_proc_job>(hdl, msg, m_params.m_post_script_config);
                    }

                protected:

                    /**
                     * Allows to schedule the incoming processor request
                     * @param hdl the connection handler to identify the session object.
                     * @param msg a pointer to the request data, not NULL
                     * @param lang_config the configuration for the language, might be undefined.
                     */
                    template<typename job_type>
                    inline void process(websocketpp::connection_hdl hdl, proc_req_in * msg,
                            const language_config & lang_config) {
                        recursive_guard guard(m_sessions_lock);

                        //Get the session id
                        session_id_type session_id = get_session_id(hdl);
                        //Get the job id
                        const string job_token = msg->get_job_token();

                        //Only process the new request if we are not stopping 
                        if (!m_is_stopping) {
                            //Get the session data associated with the session id
                            session_data & entry = m_sessions[session_id];

                            //Check if the given request already has a job associated with it.
                            proc_job_ptr & job = entry.m_jobs[job_token];

                            //If there is no job create one and add it to the map
                            if (job == NULL) {
                                //If the language is not known use the default
                                job = new job_type(lang_config, session_id, msg, m_resp_send_func);
                                LOG_DEBUG << "Got the new job: " << job << " to translate." << END_LOG;
                            } else {
                                //Add the request to the job
                                job->add_request(msg);
                            }

                            //Check if the job is ready to start
                            if (job->is_complete()) {
                                //Remove the job from the mapping
                                entry.m_jobs.erase(job_token);
                                //Schedule the complete job for execution
                                this->plan_new_job(job);
                            }
                        } else {
                            //If we are stopping then just log a message, the
                            //client will get disconnected any ways, so no harm.
                            LOG_DEBUG << "Ignoring a new processor request session_id: "
                                    << to_string(session_id) << ", job_token: " << job_token
                                    << ", the server is stopping!" << END_LOG;
                        }
                    }

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
                        //Cancel incomplete jobs
                        cancel_incomp_jobs(session_id);

                        //Cancel all the jobs from the given session, is not
                        //synchronized on the job files, the job files are not
                        //deleted then as the pre processor might get a different
                        //session than the post-processor.
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

                        //There is nothing to be done here.
                    }

                    /**
                     * Allows to cancel all of the incomplete jobs from the given session
                     * @param session_id the session to cancel the incomplete jobs for.
                     */
                    inline void cancel_incomp_jobs(session_id_type session_id) {
                        recursive_guard guard(m_sessions_lock);

                        //Get the session data
                        session_data entry = m_sessions[session_id];

                        //Iterate through the jobs and cancel them
                        for (auto iter = entry.m_jobs.begin(); iter != entry.m_jobs.end(); ++iter) {
                            iter->second->cancel();
                        }
                    }

                private:
                    //Stores the flag that indicates that we are stopping, made an atomic just in case
                    a_bool_flag m_is_stopping;
                    //Stores the reference to the set of processor parameters
                    const processor_parameters & m_params;
                    //Stores the tasks pool
                    task_pool<processor_job> m_proc_pool;
                    //Stores the reference to the function for sending the response to the client
                    const session_response_sender m_resp_send_func;
                    //Stores the mutex for accessing the incomplete jobs map 
                    recursive_mutex m_sessions_lock;
                    //Stores the incomplete jobs per session. Once a job
                    //is complete it shall be scheduled for an execution.
                    sessions_map m_sessions;
                };
            }
        }
    }
}

#endif /* PROCESSOR_MANAGER_HPP */

