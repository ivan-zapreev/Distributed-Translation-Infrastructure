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

#include "session_object.hpp"

using namespace std;

#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This is a synchronized session manager class that stores
                 * that keeps track of the open client sessions and their objects.
                 */
                class session_manager {
                public:
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                    /**
                     * The basic constructor
                     */
                    session_manager() {
                        //ToDo: Possibly limit the number of allowed open sessions (from one host and the maximum amount of allowed hosts)
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~session_manager() {
                        //ToDo: Destroy the available session objects, this should also stop any pending/running translation job
                    }

                    /**
                     * Allows to create and register a new session object, synchronized.
                     * If null is returned then it was not possible to
                     * create a new session object due to some reason.
                     * @param hdl [in] the connection handler to identify the session object.
                     * @param err_msg [out] the occurred error message, or not changed if no error
                     * @return the pointer to the newly allocation session object, or null if an error has occurred
                     */
                    session_object_ptr create_session(websocketpp::connection_hdl hdl, string & err_msg) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_lock guard(m_lock);

                        //Get the current value stored under the connection handler
                        session_object_ptr & ptr_ref = m_sessions[hdl];

                        //If the value is null then there is no session object yet, as expected
                        if (ptr_ref == NULL) {
                            //Store the pointer to the newly created connection object here
                            ptr_ref = new session_object();
                            return ptr_ref;
                        } else {
                            err_msg = "The same connection handler already exists and has a session!";
                            LOG_ERROR << err_msg << END_LOG;
                            return NULL;
                        }
                    }

                    /**
                     * Allows to get the session object associated with the connection handler, synchronized.
                     * @param hdl [in] the connection handler to identify the session object.
                     * @return 
                     */
                    session_object_ptr get_session(websocketpp::connection_hdl hdl) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_lock guard(m_lock);

                        //Get what ever it is stored
                        session_object_ptr session_ptr = m_sessions[hdl];

                        //Do a sanity check
                        ASSERT_CONDITION_THROW((session_ptr == NULL),
                                "No session object is associated with the connection handler!");

                        //If the session object is not null then return it
                        return session_ptr;
                    }

                    /**
                     * Allows to erase the session object from the map and return the stored object, synchronized.
                     * Returns NULL if there was no session object associated with the given handler.
                     * @param hdl the connection handler to identify the session object.
                     * @return the session object to be removed, is to be deallocated by the caller.
                     */
                    void destroy_session(websocketpp::connection_hdl hdl) {
                        //Use the scoped mutex lock to avoid race conditions
                        scoped_lock guard(m_lock);

                        //First get the session object pointer and destroy the session object
                        session_object_ptr ptr = m_sessions[hdl];
                        if (ptr != NULL) {
                            delete ptr;
                        }

                        //Erase the object from the map
                        m_sessions.erase(hdl);
                    }

                private:
                    //Stores the synchronization mutex accessing session objects
                    websocketpp::lib::mutex m_lock;

                    //Stores the sessions information mappings
                    map<websocketpp::connection_hdl, session_object_ptr, std::owner_less<websocketpp::connection_hdl>> m_sessions;
                };
            }
        }
    }
}

#endif /* SESSION_MANAGER_HPP */

