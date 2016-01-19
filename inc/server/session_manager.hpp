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

#include <unordered_map>

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
                    ~session_manager() {
                        //ToDo: Destroy the available session objects, this should also stop any pending/running traslation job
                    }

                    /**
                     * Allows to create and register a new session object
                     * @param hdl [in] the connection handler
                     * @param session_ptr [out] the reference to a pointer to assign the address of the newly allocation session object to 
                     * @return true if the session object has been successfully allocated, otherwise false
                     */
                    bool create_new_session(websocketpp::connection_hdl hdl, session_opject_ptr & session_ptr) {
                        //Instantiate a new session
                        session_ptr = new session_object();

                        //The next piece of code is to be executed as an atomic operation
                        {
                            //Use the scoped mutex lock to avoid race conditions
                            scoped_lock guard(m_lock_id);

                            //Get the current value stored under the connection handler
                            session_object_ptr & ptr_ref = m_session[hdl];

                            //If the value is null then there is no session object yet, as expected
                            if (ptr_ref == NULL) {
                                //Store the pointer to the newly created connection object here
                                ptr_ref = session_ptr;
                                return true;
                            } else {
                                //ToDo: Log the error as we encountered a bas situation.
                                return false;
                            }
                        }
                    }

                    /**
                     * Allows to erase the session object from the map and return the stored object.
                     *  Returns NULL if there was no session object associated with the given handler.
                     * @param hdl the session handler to identify the session object.
                     * @return the session object to be removed, is to be deallocated by the caller.
                     */
                    session_object_ptr remove_session(websocketpp::connection_hdl hdl) {
                        //The next piece of code is to be executed as an atomic operation
                        {
                            scoped_lock guard(m_lock_id);
                            //First get the session object pointer
                            session_object_ptr ptr = m_session[hdl];
                            //Erase the object from the map
                            m_sessions.erase(hdl);
                        }

                        //Return the pointer
                        return ptr;
                    }

                private:
                    //Stores the synchronization mutex accessing session objects
                    websocketpp::lib::mutex m_lock;

                    //Stores the sessions information mappings
                    unordered_map<connection_hdl, session_object_ptr, std::owner_less<connection_hdl>> m_sessions;
                };
            }
        }
    }
}

#endif /* SESSION_MANAGER_HPP */

