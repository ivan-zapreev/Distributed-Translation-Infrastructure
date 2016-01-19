/* 
 * File:   session_object.hpp
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
 * Created on January 19, 2016, 2:18 PM
 */

#include <unordered_map>

#include <websocketpp/common/thread.hpp>

#include "common/messaging/id_manager.hpp"
#include "common/messaging/translation_job_request.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef SESSION_OBJECT_HPP
#define SESSION_OBJECT_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                //Declare the session object pointer
                class session_object;
                typedef session_object * session_object_ptr;

                //Make the typedef for the translation session id
                typedef uint64_t session_id_type;

                /**
                 * This is a session object, that contains the connection information
                 * and the scheduled translation job requests information for the connection.
                 */
                class session_object {
                public:
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
                    
                    //Stores the minimum allowed session job id
                    static constexpr session_id_type MINIMUM_SESSION_ID = 1;

                    /**
                     * Allows to create a new session object for the translation client.
                     */
                    session_object() : m_session_id(m_id_mgr.get_next_id()) {
                    }

                    /**
                     * Allows to retrieve the session id associated with this session object
                     * @return the session id
                     */
                    inline const session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to add the job request to the session object
                     * @param job_request the translation job request
                     */
                    void add_job_request(translation_job_request * job_request) {
                        scoped_lock guard(m_lock);
                        
                        //ToDo: Implement
                    }

                    /**
                     * Allows to remove the job request from the session object
                     * @param job_id the id of the translation job request to be removed
                     */
                    void remove_job_request(job_id_type job_id) {
                        scoped_lock guard(m_lock);
                        
                        //ToDo: Implement
                    }

                private:
                    //Stores the synchronization mutex accessing session object data
                    websocketpp::lib::mutex m_lock;

                    //Stores the static instance of the id manager
                    static id_manager<session_id_type> m_id_mgr;

                    //Stores the session id
                    const session_id_type m_session_id;

                    //For all on-going jobs, stores the mapping from the translation
                    //job id to the resulting translation job request.
                    //The request object is owned by the session.
                    unordered_map<job_id_type, translation_job_request *> m_jobs;
                };

                constexpr session_id_type session_object::MINIMUM_SESSION_ID;

                id_manager<session_id_type> session_object::m_id_mgr(MINIMUM_SESSION_ID);
            }
        }
    }
}

#endif /* SESSION_OBJECT_HPP */

