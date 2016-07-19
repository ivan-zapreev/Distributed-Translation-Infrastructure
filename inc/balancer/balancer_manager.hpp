/* 
 * File:   balancer_manager.hpp
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
 * Created on July 7, 2016, 12:12 PM
 */

#ifndef BALANCER_MANAGER_HPP
#define BALANCER_MANAGER_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/task_pool.hpp"

#include "common/messaging/session_manager.hpp"
#include "common/messaging/session_job_pool_base.hpp"

#include "client/messaging/trans_job_resp_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"

#include "balancer/balancer_consts.hpp"
#include "balancer/balancer_parameters.hpp"
#include "balancer/translator_adapter.hpp"
#include "balancer/balancer_job.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::client::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the translation manager class:
                 * Responsibilities:
                 *      Keep track of client sessions
                 *      Keep track of client job requests
                 *      Cancel job requests for disconnected clients
                 *      Report error job in case could not dispatch
                 *      Send the translation response for a job
                 *      Maintain the translation jobs queue
                 *      Give job requests new - global job ids
                 *      Map local job id to session/trans_job data
                 *      Increase/Decrease the number of dispatchers
                 */
                class balancer_manager : public session_manager, private session_job_pool_base<balancer_job> {
                public:
                    //Declare the function that will be choosing the proper adapter for the translation job
                    typedef function<translator_adapter *(const language_uid, const language_uid) > adapter_chooser;

                    /**
                     * The basic constructor
                     * @param num_threads_incoming the number of thread to work on the incoming pool of jobs
                     * @param num_threads_outgoing the number of thread to work on the outgoing pool of jobs
                     */
                    balancer_manager(const size_t num_threads_incoming, const size_t num_threads_outgoing)
                    : session_manager(), session_job_pool_base(
                    bind(&balancer_manager::notify_job_done, this, _1)),
                    m_chooser_func(NULL), m_incoming_tasks_pool(num_threads_incoming),
                    m_outgoing_tasks_pool(num_threads_outgoing) {
                    }

                    /**
                     * Allows to set the adapter chooser function
                     * @param chooser the function needed for getting translation adapters
                     */
                    inline void set_adapter_chooser(adapter_chooser chooser) {
                        m_chooser_func = chooser;
                    }

                    /**
                     * Reports the run-time information
                     */
                    void report_run_time_info() {
                        //Report the super class info first
                        session_job_pool_base::report_run_time_info();

                        //Report data from the task pools
                        m_incoming_tasks_pool.report_run_time_info("Incoming tasks pool");
                        m_outgoing_tasks_pool.report_run_time_info("Outgoing tasks pool");
                    }

                    /**
                     * Allows to stop the manager
                     */
                    inline void stop() {
                        session_job_pool_base<balancer_job>::stop();
                    }
                    
                    /**
                     * Shall be called when a new translation job response arrives from a translation server adapter.
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    inline void notify_translation_response(trans_job_resp_in * trans_job_resp) {
                        //ToDo: Implement handling of the translation job response
                    }

                    /**
                     * Allows to process the server translation job request message
                     * @param hdl the connection handler to identify the session object.
                     * @param trans_req a pointer to the translation job request data, not NULL
                     */
                    inline void translate(websocketpp::connection_hdl hdl, trans_job_req_in * trans_req) {
                        //Get the session id
                        session_id_type session_id = get_session_id(hdl);

                        LOG_DEBUG << "Received a translation request from session: " << session_id << END_LOG;

                        //Check that there is a session mapped to this handler
                        ASSERT_CONDITION_THROW((session_id == session_id::UNDEFINED_SESSION_ID),
                                "No session object is associated with the connection handler!");

                        //Instantiate a new translation job, it will destroy the translation request in its destructor
                        bal_job_ptr job = new balancer_job(session_id, trans_req);

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

                    /**
                     * Shall be called when a server's adapter gets disconnected. In this case all the
                     * jobs being awaiting response from this adapter are to be canceled.
                     * @param uid the unique identifier of the adapter
                     */
                    inline void notify_adapter_disconnect(const trans_server_uid & uid) {
                        //ToDo: Implement cancellation of all the translation requests which have 
                        //      been sent but the response was not received yet.
                    }

                protected:

                    /**
                     * Will be called once a new job is scheduled. Here we need to perform
                     * the needed actions with the job. I.e. add it into the incoming tasks pool.
                     * @see session_job_pool_base
                     */
                    virtual void process_new_job(bal_job_ptr bal_job) {
                        m_incoming_tasks_pool.plan_new_task(bal_job);
                    }
                    
                    /**
                     * Allows to set the non-error translation result,
                     * this will also send the response to the client.
                     * @param bal_job the pointer to the finished translation job 
                     */
                    inline void notify_job_done(bal_job_ptr bal_job) {
                        //ToDo: Implement
                    }

                    /**
                     * Will be called when a session with the given id is closed
                     * @see session_manager
                     */
                    virtual void session_is_closed(session_id_type session_id) {
                        //Cancel all the jobs from the given session
                        this->cancel_jobs(session_id);
                    }

                private:
                    //Stores the function for choosing the adapter
                    adapter_chooser m_chooser_func;
                    //Stores the tasks pool
                    task_pool<balancer_job> m_incoming_tasks_pool;
                    //Stores the tasks pool
                    task_pool<balancer_job> m_outgoing_tasks_pool;
                };

            }
        }
    }
}

#endif /* BALANCER_MANAGER_HPP */

