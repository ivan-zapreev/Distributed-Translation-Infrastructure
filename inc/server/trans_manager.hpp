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

#include <map>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/server.hpp>

#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/id_manager.hpp"
#include "trans_session.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This is a synchronized translation sessions manager class that stores
                 * that keeps track of the open translation sessions and their objects.
                 */
                class trans_manager {
                public:
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
                    typedef websocketpp::lib::function<void(websocketpp::connection_hdl, trans_job_response &) > response_sender;
                    typedef std::map<websocketpp::connection_hdl, session_id_type, std::owner_less<websocketpp::connection_hdl>> sessions_map_type;
                    typedef std::map<session_id_type, websocketpp::connection_hdl> handlers_map_type;

                    /**
                     * The basic constructor
                     */
                    trans_manager() : m_session_id_mgr(session::MINIMUM_SESSION_ID) {
                        //ToDo: Possibly limit the number of allowed open sessions (from one host and the maximum amount of allowed hosts)
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
                        //Request stopping of all the running translation jobs
                        for (handlers_map_type::iterator iter;
                                iter != m_handlers.end(); ++iter) {
                            //Cancel the translation jobs for the given session id
                            cancel_trans_job_requests(iter->first);
                        }
                    }

                    /**
                     * Allows to create and register a new session object, synchronized.
                     * If for some reason a new session can not be opened, an exception is thrown.
                     * @param hdl [in] the connection handler to identify the session object.
                     */
                    void open_session(websocketpp::connection_hdl hdl) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_lock guard(m_lock);

                        //Get the current value stored under the connection handler
                        session_id_type & session_id = m_sessions[hdl];

                        //Do a sanity check, that the session id is yet undefined
                        ASSERT_SANITY_THROW((session_id != session::UNDEFINED_SESSION_ID),
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
                        session_id_type session_id = session::UNDEFINED_SESSION_ID;

                        //Use the scoped mutex lock to avoid race conditions
                        {
                            scoped_lock guard(m_lock);

                            //Get what ever it is stored
                            session_id = m_sessions[hdl];
                        }

                        //Do a sanity check, that there is a session object
                        ASSERT_SANITY_THROW((session_id == session::UNDEFINED_SESSION_ID),
                                "No session object is associated with the connection handler!");

                        //ToDo: Schedule a translation job request for the session id
                    }

                    /**
                     * Allows to erase the session object from the map and return the stored object, synchronized.
                     * Returns NULL if there was no session object associated with the given handler.
                     * @param hdl the connection handler to identify the session object.
                     * @return the session object to be removed, is to be deallocated by the caller.
                     */
                    void close_session(websocketpp::connection_hdl hdl) {
                        //Declare the session id vari/home/zapreevis/Projects/Basic-Phrase-Based-Decoding/inc/server/trans_manager.hpp:60:56: error: ‘MINIMUM_SESSION_ID’ was not declared in this scopeable
                        session_id_type session_id = session::UNDEFINED_SESSION_ID;

                        //Use the scoped mutex lock to avoid race conditions
                        {
                            scoped_lock guard(m_lock);

                            //Get the session id from the handler
                            session_id_type session_id = m_sessions[hdl];

                            //Erase the handler and session mappings
                            m_sessions.erase(hdl);
                            m_handlers.erase(session_id);
                        }

                        //Request cancellation of all the translation jobs associated with this connection.
                        if (session_id != session::UNDEFINED_SESSION_ID) {
                            //NOTE: This can be done outside the synchronization block
                            cancel_trans_job_requests(session_id);
                        }
                    }

                protected:

                    /**
                     * Cancel all translation job requests for the given session
                     * @param session_id the session id to cancel the translation jobs for.
                     */
                    void cancel_trans_job_requests(const session_id_type session_id) {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to set the non-error translation result,
                     * this will also send the response to the client.
                     * @param session_id the session id
                     * @param job_id the job id
                     * @param text the translated text
                     */
                    void set_translation(const session_id_type session_id, const job_id_type job_id, const string & text) {
                        //Create the translation job response
                        trans_job_response response(job_id, job_result_code::RESULT_OK, text);

                        //Do the sanity check assert
                        ASSERT_SANITY_THROW(!m_sender_func,
                                "The sender function of the translation manager is not set!");

                        //Declare the connection handler
                        websocketpp::connection_hdl hdl;

                        //Retrieve the connection handler based on the session id
                        {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_lock guard(m_lock);

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
                    //Stores the static instance of the id manager
                    id_manager<session_id_type> m_session_id_mgr;

                    //Stores the reply sender functional
                    response_sender m_sender_func;

                    //Stores the synchronization mutex
                    websocketpp::lib::mutex m_lock;

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

