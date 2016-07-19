/* 
 * File:   session_manager.hpp
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
 * Created on January 19, 2016, 4:02 PM
 */

#ifndef TRANSLATION_MANAGER_HPP
#define TRANSLATION_MANAGER_HPP

#include <functional>

#include "server/trans_job.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/task_pool.hpp"

#include "common/messaging/session_manager.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/session_job_pool_base.hpp"

#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

using namespace std;
using namespace std::placeholders;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This is a synchronized translation sessions manager class that stores
                 * that keeps track of the open translation sessions and their objects.
                 */
                class translation_manager : public session_manager, private session_job_pool_base<trans_job> {
                public:

                    /**
                     * The basic constructor.
                     * @param num_threads the number of translation threads to run
                     */
                    translation_manager(const size_t num_threads)
                    : session_manager(), session_job_pool_base(
                    bind(&translation_manager::notify_job_done, this, _1)),
                    m_tasks_pool(num_threads) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~translation_manager() {
                        //No need to do anything the manager is destroyed only when the application is stopped.
                        //The scheduled jobs will be canceled by the by the trans_job_pool destructor
                    }

                    /**
                     * Allows to set the new number of worker threads.
                     * This operation should be safe as the new threads
                     * are just added to the list and the deleted ones
                     * are let to finish their translation task execution. 
                     * @param num_threads the new number of worker threads
                     */
                    void set_num_threads(const size_t num_threads) {
                        m_tasks_pool.set_num_threads(num_threads);
                    }

                    /**
                     * Allows to report the runtime information.
                     */
                    void report_run_time_info() {
                        //Report the super class info first
                        session_job_pool_base::report_run_time_info();

                        //Report data from the tasks pool
                        m_tasks_pool.report_run_time_info("Translation tasks pool");
                    }

                    /**
                     * Allows to stop the manager
                     */
                    inline void stop() {
                        session_job_pool_base<trans_job>::stop();
                    }
                    
                    /**
                     * Allows to schedule a new translation request, synchronized.
                     * If there is not session associated with the given connection
                     * handler then will through. The scheduled translation job
                     * request is from this moment on a responsibility of the
                     * underlying object to be managed.
                     * @param hdl [in] the connection handler to identify the session object.
                     * @param trans_req [in] the translation job request reference
                     */
                    void translate(websocketpp::connection_hdl hdl, trans_job_req_in & trans_req) {
                        //Get the session id
                        session_id_type session_id = get_session_id(hdl);

                        LOG_DEBUG << "Received a translation request from session: " << session_id << END_LOG;

                        //Check that there is a session mapped to this handler
                        ASSERT_CONDITION_THROW((session_id == session_id::UNDEFINED_SESSION_ID),
                                "No session object is associated with the connection handler!");

                        //Instantiate a new translation job, it will destroy the translation request in its destructor
                        trans_job_ptr job = new trans_job(session_id, trans_req);

                        LOG_DEBUG << "Got the new job: " << job << " to translate." << END_LOG;

                        try {
                            //Schedule a translation job request for the session id
                            this->plan_new_job(job);
                        } catch (std::exception & ex) {
                            //Catch any possible exception and delete the translation job
                            LOG_ERROR << ex.what() << END_LOG;
                            if (job != NULL) {
                                delete job;
                            }
                            //Re-throw the exception
                            throw ex;
                        }
                    }

                protected:

                    /**
                     * @see session_manager
                     */
                    virtual void session_is_closed(session_id_type session_id) {
                        //Cancel the jobs from this session
                        this->cancel_jobs(session_id);
                    }

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

                    /**
                     * Allows to set the non-error translation result,
                     * this will also send the response to the client.
                     * @param trans_job the pointer to the finished translation job 
                     */
                    inline void notify_job_done(trans_job_ptr trans_job) {
                        //Declare and initialize session and job id for future use
                        const job_id_type job_id = trans_job->get_job_id();
                        const job_id_type session_id = trans_job->get_session_id();

                        LOG_DEBUG << "Finished job id: " << job_id << ", session: " << session_id << END_LOG;

                        //Create the translation job response
                        trans_job_resp_out response;

                        //Populate the translation job response with the data
                        trans_job->collect_job_results(response);

                        //Attempt to send the serialized response
                        if (!this->send_response(session_id, response.serialize())) {
                            LOG_DEBUG << "Could not send the translation response for " << session_id
                                    << "/" << job_id << " as the connection handler has expired!" << END_LOG;
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

#endif /* SESSION_MANAGER_HPP */

