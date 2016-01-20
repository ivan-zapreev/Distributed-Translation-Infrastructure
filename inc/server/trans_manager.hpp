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

#include "trans_session.hpp"
#include "common/messaging/trans_job_request.hpp"

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

                    /**
                     * The basic constructor
                     */
                    trans_manager() {
                        //ToDo: Possibly limit the number of allowed open sessions (from one host and the maximum amount of allowed hosts)
                    }

                    /**
                     * Allows to set the response sender function for sending the replies to the client
                     * @param sender the s ender functional to be set
                     */
                    void set_response_sender(response_sender sender) {
                        m_sender = sender;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_manager() {
                        //ToDo: Destroy the available session objects, this should also stop any pending/running translation job
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
                        trans_session_ptr & ptr_ref = m_sessions[hdl];

                        //Do a sanity check, that there is no session object yet
                        ASSERT_CONDITION_THROW((ptr_ref != NULL),
                                "The same connection handler already exists and has a session!");

                        //Store the pointer to the newly created connection object here
                        ptr_ref = new trans_session();
                    }

                    /**
                     * Allows to schedule a new translation request, synchronized.
                     * If there is not session associated with the given connection
                     * handler then will through. The scheduled translation job
                     * request is from this moment on a responsibility of the
                     * underlying trans_session_object to be managed.
                     * @param hdl [in] the connection handler to identify the session object.
                     * @param request_ptr [in] the translation job request to be stored, not NULL
                     */
                    void translate(websocketpp::connection_hdl hdl, trans_job_request_ptr request_ptr) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_lock guard(m_lock);

                        //Get what ever it is stored
                        trans_session_ptr ptr = m_sessions[hdl];

                        //Do a sanity check, that there is a session object
                        ASSERT_CONDITION_THROW((ptr == NULL),
                                "No session object is associated with the connection handler!");

                        //Schedule a translation job request
                        ptr->add_job_request(request_ptr);
                    }

                    /**
                     * Allows to erase the session object from the map and return the stored object, synchronized.
                     * Returns NULL if there was no session object associated with the given handler.
                     * @param hdl the connection handler to identify the session object.
                     * @return the session object to be removed, is to be deallocated by the caller.
                     */
                    void close_session(websocketpp::connection_hdl hdl) {
                        trans_session_ptr ptr = NULL;

                        //In the next block we take the session object and erase it from the map.
                        //The deletion of the object can then be done in an unsynchronized way.
                        {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_lock guard(m_lock);

                            //Get the session object pointer
                            ptr = m_sessions[hdl];

                            //Erase the object from the map
                            m_sessions.erase(hdl);
                        }

                        if (ptr != NULL) {
                            //Destroy the session object
                            delete ptr;
                        }
                    }

                private:
                    //Stores the reply sender functional
                    response_sender m_sender;
                    //Stores the synchronization mutex accessing session objects
                    websocketpp::lib::mutex m_lock;

                    //Stores the connection handler to sessions information mappings
                    map<websocketpp::connection_hdl, trans_session_ptr, std::owner_less<websocketpp::connection_hdl>> m_sessions;
                };
            }
        }
    }
}

#endif /* SESSION_MANAGER_HPP */

