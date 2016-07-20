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
 * Created on July 15, 2016, 1:15 PM
 */

#include <map>
#include <unordered_map>
#include <functional>

#include <websocketpp/server.hpp>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"
#include "common/utils/string_utils.hpp"
#include "common/utils/id_manager.hpp"

#include "common/messaging/trans_session_id.hpp"
#include "msg_base.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::common::messaging;

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is a synchronized translation sessions manager class that stores
                     * that keeps track of the open translation sessions and their objects.
                     * 
                     * \todo {Possibly limit the number of allowed open sessions
                     * (from one host and the maximum amount of allowed hosts)
                     * This is for later, if the server is put for www access.}
                     */
                    class session_manager {
                    public:

                        //Declare the response setting function for the translation job.
                        typedef function<void(websocketpp::connection_hdl, const string &) > response_sender;

                        //Declare the session to connection handler maps and iterators;
                        typedef std::map<websocketpp::connection_hdl, session_id_type, std::owner_less<websocketpp::connection_hdl>> sessions_map_type;
                        typedef std::unordered_map<session_id_type, websocketpp::connection_hdl> handlers_map_type;
                        typedef handlers_map_type::iterator handlers_map_iter_type;

                        /**
                         * The basic class constructor
                         */
                        session_manager()
                        : m_session_id_mgr(session_id::MINIMUM_SESSION_ID) {
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~session_manager() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to set the response sender function for sending the replies to the client
                         * @param sender the s ender functional to be set
                         */
                        inline void set_response_sender(response_sender sender) {
                            m_sender_func = sender;
                        }

                        /**
                         * Allows to create and register a new session object, synchronized.
                         * If for some reason a new session can not be opened, an exception is thrown.
                         * @param hdl [in] the connection handler to identify the session object.
                         */
                        inline void open_session(websocketpp::connection_hdl hdl) {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_guard guard(m_lock);

                            //Get the current value stored under the connection handler
                            session_id_type & session_id = m_sessions[hdl];

                            //Do a sanity check, that the session id is yet undefined
                            if (session_id == session_id::UNDEFINED_SESSION_ID) {
                                //Issue a session id to that new connection!
                                session_id = m_session_id_mgr.get_next_id();
                                //Add the handler to session id mapping
                                m_handlers[session_id] = hdl;
                            } else {
                                LOG_WARNING << "The same connection handler already exists and has a session!" << END_LOG;
                            }
                        }

                        /**
                         * Allows to erase the session object from the map and return the stored object, synchronized.
                         * Returns NULL if there was no session object associated with the given handler.
                         * @param hdl the connection handler to identify the session object.
                         * @return the session object to be removed, is to be deallocated by the caller.
                         */
                        inline void close_session(websocketpp::connection_hdl hdl) {
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

                            //Notify about the closed session.
                            if (session_id != session_id::UNDEFINED_SESSION_ID) {
                                session_is_closed(session_id);
                            }

                            LOG_DEBUG << "The session with id: " << session_id << " is closed!" << END_LOG;
                        }

                    protected:

                        /**
                         * Allows to send the response from the given session
                         * @param session_id the session id
                         * @param msg the message to be sent
                         * @return true if the sent was successful, otherwise false
                         */
                        inline bool send_response(const session_id_type session_id, const msg_base & msg) {
                            LOG_DEBUG << "Sending a message from session: " << to_string(session_id) << END_LOG;
                            
                            //Do the sanity check assert
                            ASSERT_SANITY_THROW(!m_sender_func,
                                    "The sender function of the translation manager is not set!");

                            //Serialize the message
                            const string data = msg.serialize();
                            
                            //Retrieve the connection handler based on the session id
                            websocketpp::connection_hdl hdl = get_session_hdl(session_id);

                            //If the sender function is present, and the handler is not expired
                            if (!hdl.expired()) {
                                LOG_DEBUG << "Sending translation job response: " << data << END_LOG;

                                //Send the response to the client
                                m_sender_func(hdl, data);

                                //The send was successful
                                return true;
                            } else {
                                LOG_DEBUG << "ERROR: Could not send the translation "
                                        << "response to session " << to_string(session_id)
                                        << " the connection handler has expired!" << END_LOG;
                                //The send failed
                                return false;
                            }
                        }

                        /**
                         * Will be called one the session is closed
                         * @param session_id the id of the closed session
                         */
                        virtual void session_is_closed(session_id_type session_id) = 0;

                        /**
                         * ALlows to get a session id for the given handler
                         * @param hdl the session handler
                         * @return the session id, if registered, otherwise session_id::UNDEFINED_SESSION_ID
                         */
                        inline session_id_type get_session_id(const websocketpp::connection_hdl hdl) {
                            scoped_guard guard(m_lock);

                            //Get what ever it is stored
                            return m_sessions[hdl];
                        }

                        /**
                         * Allows to get the session handler for the given session id
                         * @param session_id the session id
                         * @return the corresponding session handler
                         */
                        inline websocketpp::connection_hdl & get_session_hdl(const session_id_type session_id) {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_guard guard(m_lock);

                            //Get the connection handler for the session
                            return m_handlers[session_id];
                        }

                    private:
                        //Stores the reply sender functional
                        response_sender m_sender_func;

                        //Stores the instance of the id manager
                        id_manager<session_id_type> m_session_id_mgr;

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
}

#endif /* SESSION_MANAGER_HPP */

