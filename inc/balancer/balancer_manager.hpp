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

#include <unordered_map>
#include <set>

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
using namespace std::placeholders;

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
                    //Define the map for relating the balancer job ids with the jobs
                    typedef unordered_map<job_id_type, bal_job_ptr> server_jobs_map_type;

                    /**
                     * Defined the structure storing the jobs set and the mutex to synchronize access to it
                     */
                    typedef struct {
                        //The mapping of the balancer job id to job for the jobs awaiting server reply
                        server_jobs_map_type m_awaiting_jobs;
                        //The mutex to synchronize access to the set
                        mutex m_awaiting_jobs_lock;
                    } server_jobs_entry_type;

                    //Define the awaiting reply jobs map, this map relates the adapter
                    //id to the set of jobs awaiting a response from this adapter
                    typedef unordered_map<server_id_type, server_jobs_entry_type> awaiting_jobs_map;

                    //Define the map type for storing the relation between the balancer job ids and the balancer jobs
                    typedef unordered_map<job_id_type, bal_job_ptr> bal_jobs_map;

                    /**
                     * The basic constructor
                     * @param num_threads_incoming the number of thread to work on the incoming pool of jobs
                     * @param num_threads_outgoing the number of thread to work on the outgoing pool of jobs
                     */
                    balancer_manager(const size_t num_threads_incoming, const size_t num_threads_outgoing)
                    : session_manager(), session_job_pool_base(
                    bind(&balancer_manager::notify_job_done, this, _1)),
                    m_choose_adapt_func(NULL),
                    m_register_wait_func(bind(&balancer_manager::register_awaiting_resp, this, _1)),
                    m_schedule_failed_func(bind(&balancer_manager::schedule_failed_job_response, this, _1)),
                    m_resp_send_func(bind(&balancer_manager::send_response, this, _1, _2)),
                    m_incoming_pool(num_threads_incoming),
                    m_outgoing_pool(num_threads_outgoing) {
                    }

                    /**
                     * Allows to set the adapter chooser function
                     * @param chooser the function needed for getting translation adapters
                     */
                    inline void set_adapter_chooser(adapter_chooser chooser) {
                        m_choose_adapt_func = chooser;
                    }

                    /**
                     * Reports the run-time information
                     */
                    void report_run_time_info() {
                        //Report the super class info first
                        session_job_pool_base::report_run_time_info();

                        //Report data from the task pools
                        m_incoming_pool.report_run_time_info("Incoming tasks pool");
                        m_outgoing_pool.report_run_time_info("Outgoing tasks pool");
                    }

                    /**
                     * Allows to stop the manager
                     */
                    inline void stop() {
                        session_job_pool_base<balancer_job>::stop();
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
                        bal_job_ptr job = new balancer_job(
                                session_id, trans_req,
                                m_choose_adapt_func, m_register_wait_func,
                                m_schedule_failed_func, m_resp_send_func);

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
                     * Shall be called when a new translation job response arrives from a translation server adapter.
                     * @param id the id of the server adapter we get the response from.
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    inline void notify_translation_response(const server_id_type server_id, trans_job_resp_in * trans_job_resp) {
                        //Declare the balancer job pointer variable
                        bal_job_ptr bal_job = NULL;
                        //Get the job id
                        const job_id_type bal_job_id = trans_job_resp->get_job_id();

                        //ToDo: Pass the server id with the response, then we can remove
                        //the m_awaiting_bi2j map and only use the m_awaiting_a2j

                        LOG_DEBUG << "Got translation job response: " << to_string(bal_job_id) << END_LOG;

                        //Get the server jobs entry
                        server_jobs_entry_type& entry = get_server_jobs(server_id);

                        //Remove the job from the set
                        {
                            unique_guard guard(entry.m_awaiting_jobs_lock);

                            //Search for the job by its id
                            auto iter = entry.m_awaiting_jobs.find(bal_job_id);

                            //Check if the job is found
                            if (iter != entry.m_awaiting_jobs.end()) {
                                //Get the balancer job
                                bal_job = iter->second;
                            }
                        }

                        //Check if the job is found
                        if (bal_job != NULL) {
                            //Set the translation response into the job
                            bal_job->set_trans_job_resp(trans_job_resp);

                            //Just put the job into the outgoing pool.
                            m_outgoing_pool.plan_new_task(bal_job);
                        } else {
                            LOG_DEBUG << "The balancer job: " << to_string(bal_job_id) << " is no "
                                    << "longer present, the server response is ignored!" << END_LOG;
                        }
                    }

                    /**
                     * Shall be called when a server's adapter gets disconnected. In this case all the
                     * jobs being awaiting response from this adapter are to be canceled.
                     * @param id the unique identifier of the adapter
                     */
                    inline void notify_adapter_disconnect(const server_id_type & id) {
                        //Get the server jobs entry
                        server_jobs_entry_type& entry = get_server_jobs(id);

                        //Remove the job from the set
                        {
                            unique_guard guard(entry.m_awaiting_jobs_lock);

                            //Iterate through the jobs and mark them as failed.
                            for (auto iter = entry.m_awaiting_jobs.begin(); iter != entry.m_awaiting_jobs.end(); ++iter) {
                                iter->second->fail();
                            }
                        }
                    }

                protected:

                    /**
                     * Will be called once a new job is scheduled. Here we need to perform
                     * the needed actions with the job. I.e. add it into the incoming tasks pool.
                     * @see session_job_pool_base
                     */
                    virtual void schedule_new_job(bal_job_ptr bal_job) {
                        //Plan the job in to the incoming tasks pool and move on.
                        m_incoming_pool.plan_new_task(bal_job);
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
                     * Allows to get the set of server jobs.
                     * @param id the server id
                     * @return the reference to the server jobs entry
                     */
                    inline server_jobs_entry_type& get_server_jobs(server_id_type id) {
                        unique_guard guard(m_awaiting_a2j_lock);

                        //Get the pointer to the entry
                        return m_awaiting_a2j[id];
                    }

                    /**
                     * This function will be called once the balancer job is fully
                     * done an it is about to be destroyed. This function can be
                     * used to remove the job from the internal mappings.
                     * @param bal_job the balancer job that is fully done
                     */
                    inline void notify_job_done(bal_job_ptr bal_job) {
                        //If the server id is set, then let's look for the job in the mappings
                        if (bal_job->get_server_id() != server_id::UNDEFINED_SERVER_ID) {
                            //Get the server jobs entry
                            server_jobs_entry_type& entry = get_server_jobs(bal_job->get_server_id());

                            //Remove the job from the set
                            {
                                unique_guard guard(entry.m_awaiting_jobs_lock);

                                entry.m_awaiting_jobs.erase(bal_job->get_bal_job_id());
                            }
                        } else {
                            LOG_DEBUG << "Deleting an unsent translation job: " << bal_job->get_job_id() << END_LOG;
                        }
                    }

                    /**
                     * This function will be called once the balancer job is awaiting
                     * for a response from the translation server. This function is
                     * called only if the request was successfully sent to the translator. 
                     * @param bal_job pointer to the constant balancer job
                     */
                    inline void register_awaiting_resp(balancer_job * bal_job) {
                        //If the server id is set, then let's look for the job in the mappings
                        if (bal_job->get_server_id() != server_id::UNDEFINED_SERVER_ID) {
                            //Get the server jobs entry
                            server_jobs_entry_type& entry = get_server_jobs(bal_job->get_server_id());

                            //Add the new job to the set
                            {
                                unique_guard guard(entry.m_awaiting_jobs_lock);

                                entry.m_awaiting_jobs[bal_job->get_bal_job_id()] = bal_job;
                            }
                        } else {
                            LOG_ERROR << "Trying to register a job with no server id: " << *bal_job << END_LOG;
                        }
                    }

                    /**
                     * This function is called once it is clear that the job should
                     * not be waiting for the server response any more. I.e. it is
                     * time to just shuffle the job into the outgoing pool
                     * @param bal_job pointer to the constant balancer job
                     */
                    inline void schedule_failed_job_response(balancer_job * bal_job) {
                        //Just put the job into the outgoing pool.
                        m_outgoing_pool.plan_new_task(bal_job);
                    }

                private:
                    //Stores the function for choosing the adapter
                    adapter_chooser m_choose_adapt_func;
                    //Stores the function for registering a response awaiting function
                    const job_notifier m_register_wait_func;
                    //Stores the function for notifying about the error response
                    const job_notifier m_schedule_failed_func;
                    //Stores the reference to the function for sending the translation response to the client
                    const session_response_sender m_resp_send_func;

                    //Stores the tasks pool
                    task_pool<balancer_job> m_incoming_pool;
                    //Stores the tasks pool
                    task_pool<balancer_job> m_outgoing_pool;

                    //The map of adapter ids to the awaiting response jobs
                    awaiting_jobs_map m_awaiting_a2j;

                    //The mutex to synchronize access to the map of adapter ids to the awaiting response jobs
                    mutex m_awaiting_a2j_lock;
                };

            }
        }
    }
}

#endif /* BALANCER_MANAGER_HPP */

