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

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <map>

#include <websocketpp/server.hpp>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/id_manager.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "trans_job_pool.hpp"
#include "trans_job.hpp"

using namespace std;
using namespace std::placeholders;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This is a synchronized translation sessions manager class that stores
                 * that keeps track of the open translation sessions and their objects.
                 */
                class trans_manager {
                public:

                    //Declare the response setting function for the translation job.
                    typedef function<void(websocketpp::connection_hdl, trans_job_response &) > response_sender;

                    //Declare the session to connection handler maps and iterators;
                    typedef std::map<websocketpp::connection_hdl, session_id_type, std::owner_less<websocketpp::connection_hdl>> sessions_map_type;
                    typedef std::map<session_id_type, websocketpp::connection_hdl> handlers_map_type;
                    typedef handlers_map_type::iterator handlers_map_iter_type;

                    /**
                     * The basic constructor.
                     * @param num_threads the number of translation threads to run
                     * 
                     * ToDo: Possibly limit the number of allowed open sessions
                     * (from one host and the maximum amount of allowed hosts)
                     * This is for later, if the server is put for www access.
                     */
                    trans_manager(const size_t num_threads)
                    : m_job_pool(num_threads), m_session_id_mgr(session_id::MINIMUM_SESSION_ID) {
                        //Set the response sender function into the pool
                        m_job_pool.set_job_result_setter(bind(&trans_manager::notify_job_finished, this, _1));
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param sender the s ender functional to be set
                     */
                    void set_response_sender(response_sender sender) {
                        m_sender_func = sender;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_manager() {
                        //No need to do anything the manager is destroyed only when the application is stopped.
                        //The scheduled jobs will be canceled by the by the trans_job_pool destructor
                    }
                    
                    /**
                     * Allows to report the runtime information.
                     */
                    void report_run_time_info() {
                        //Report data from the jobs pool
                        m_job_pool.report_run_time_info();
                    }
                    
                    /**
                     * Allows to create and register a new session object, synchronized.
                     * If for some reason a new session can not be opened, an exception is thrown.
                     * @param hdl [in] the connection handler to identify the session object.
                     */
                    void open_session(websocketpp::connection_hdl hdl) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_guard guard(m_lock);

                        //Get the current value stored under the connection handler
                        session_id_type & session_id = m_sessions[hdl];

                        //Do a sanity check, that the session id is yet undefined
                        ASSERT_SANITY_THROW((session_id != session_id::UNDEFINED_SESSION_ID),
                                "The same connection handler already exists and has a session!");

                        //Issue a session id to that new connection!
                        session_id = m_session_id_mgr.get_next_id();

                        //Add the handler to session id mapping
                        m_handlers[session_id] = hdl;
                    }

                    /**
                     * Allows to schedule a new translation request, synchronized.
                     * If there is not session associated with the given connection
                     * handler then will through. The scheduled translation job
                     * request is from this moment on a responsibility of the
                     * underlying object to be managed.
                     * @param hdl [in] the connection handler to identify the session object.
                     * @param request_ptr [in] the translation job request to be stored, not NULL
                     */
                    void translate(websocketpp::connection_hdl hdl, trans_job_request_ptr request_ptr) {
                        //Declare the session id variable
                        session_id_type session_id = session_id::UNDEFINED_SESSION_ID;

                        //Use the scoped mutex lock to avoid race conditions
                        {
                            scoped_guard guard(m_lock);

                            //Get what ever it is stored
                            session_id = m_sessions[hdl];
                        }

                        LOG_DEBUG << "Received a translation request from session: " << session_id << END_LOG;

                        //Check that there is a session mapped to this handler
                        ASSERT_CONDITION_THROW((session_id == session_id::UNDEFINED_SESSION_ID),
                                "No session object is associated with the connection handler!");

                        //Set the session id into the request
                        request_ptr->set_session_id(session_id);

                        //Instantiate a new translation job
                        trans_job_ptr job = new trans_job(request_ptr);

                        LOG_DEBUG << "Got the new job: " << job << " to translate." << END_LOG;

                        //Schedule a translation job request for the session id
                        m_job_pool.plan_new_job(job);
                    }

                    /**
                     * Allows to erase the session object from the map and return the stored object, synchronized.
                     * Returns NULL if there was no session object associated with the given handler.
                     * @param hdl the connection handler to identify the session object.
                     * @return the session object to be removed, is to be deallocated by the caller.
                     */
                    void close_session(websocketpp::connection_hdl hdl) {
                        LOG_DEBUG << "A closing session request!" << END_LOG;

                        //Declare the session id 
                        session_id_type session_id = session_id::UNDEFINED_SESSION_ID;

                        //Use the scoped mutex lock to avoid race conditions
                        {
                            scoped_guard guard(m_lock);

                            //Get the session id from the handler
                            session_id = m_sessions[hdl];

                            LOG_DEBUG << "A closing session request for session id: " << session_id << END_LOG;

                            //Erase the handler and session mappings
                            m_sessions.erase(hdl);
                            if (session_id != session_id::UNDEFINED_SESSION_ID) {
                                m_handlers.erase(session_id);
                            }
                        }
                        LOG_DEBUG << "Session : " << session_id << " internal mappings "
                                << "are cleaned, canceling the job! " << END_LOG;

                        //Request cancellation of all the translation jobs associated with this connection.
                        if (session_id != session_id::UNDEFINED_SESSION_ID) {
                            //NOTE: This can be done outside the synchronization block
                            m_job_pool.cancel_jobs(session_id);
                        }

                        LOG_DEBUG << "The session with id: " << session_id << " is closed!" << END_LOG;
                    }

                    /**
                     * Allows to stop the translation manager, i.e. cancel all the jobs and move on.
                     */
                    void stop() {
                        //Stop the job's pool, this is blocking until all the jobs are stopped
                        m_job_pool.stop();
                    }

                protected:

                    /**
                     * Allows to set the non-error translation result,
                     * this will also send the response to the client.
                     * @param trans_job the pointer to the finished translation job 
                     */
                    void notify_job_finished(trans_job_ptr trans_job) {
                        //Declare and initialize session and job id for future use
                        const job_id_type job_id = trans_job->get_job_id();
                        const job_id_type session_id = trans_job->get_session_id();

                        //Create the translation job response
                        trans_job_response response(job_id, trans_job->get_code(), trans_job->get_text());

                        LOG_DEBUG << "Created the job response: " << &response << " for job "
                                << trans_job->get_job_id() << " from session "
                                << trans_job->get_session_id() << END_LOG;

                        //Do the sanity check assert
                        ASSERT_SANITY_THROW(!m_sender_func,
                                "The sender function of the translation manager is not set!");

                        //Declare the connection handler
                        websocketpp::connection_hdl hdl;

                        //Retrieve the connection handler based on the session id
                        {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_guard guard(m_lock);

                            //Get the connection handler for the session
                            hdl = m_handlers[session_id];
                        }

                        //If the sender function is present, and the handler is not expired
                        if (!hdl.expired()) {
                            m_sender_func(hdl, response);
                        } else {
                            LOG_ERROR << "Could not send the translation response for " << session_id
                                    << "/" << job_id << "as the connection handler has expired!" << END_LOG;
                        }
                    }

                private:
                    //Stores the translation job pool
                    trans_job_pool m_job_pool;

                    //Stores the instance of the id manager
                    id_manager<session_id_type> m_session_id_mgr;

                    //Stores the reply sender functional
                    response_sender m_sender_func;

                    //Stores the synchronization mutex
                    mutex m_lock;

                    //Stores the connection handler to session id mappings
                    sessions_map_type m_sessions;

                    //Stores the session id to connection handler mappings
                    handlers_map_type m_handlers;
                };
            }
        }
    }
}

#endif /* SESSION_MANAGER_HPP */

