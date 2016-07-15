/* 
 * File:   translation_manager.hpp
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

#ifndef TRANSLATION_MANAGER_HPP
#define TRANSLATION_MANAGER_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/session_manager.hpp"

#include "client/messaging/trans_job_resp_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"

#include "balancer/balancer_consts.hpp"
#include "balancer/balancer_parameters.hpp"
#include "balancer/translation_job.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

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
                class translation_manager : public session_manager {
                public:

                    //Declare the response setting function for the translation job.
                    typedef function<void(websocketpp::connection_hdl, const string &) > response_sender;

                    /**
                     * The basic constructor
                     * @param params the parameters from which the server will be configured
                     * @param sender the s ender functional to be set
                     */
                    translation_manager(const balancer_parameters & params, response_sender sender)
                    : m_params(params), m_sender_func(sender) {
                    }
                    
                    /**
                     * Allows to stop the translation manager
                     */
                    inline void stop() {
                        //ToDo: Implement
                    }
                    
                    /**
                     * Reports the run-time information
                     */
                    inline void report_run_time_info() {
                        //ToDo: Implement
                    }
                    
                    /**
                     * Allows to process the server translation job response message
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    inline void register_translation_response(trans_job_resp_in * trans_job_resp) {
                        //ToDo: Implement handling of the translation job response
                    }

                    /**
                     * Allows to process the server translation job request message
                     * @param hdl the connection handler to identify the session object.
                     * @param trans_job_req a pointer to the translation job request data, not NULL
                     */
                    inline void register_translation_request(websocketpp::connection_hdl hdl, trans_job_req_in * trans_job_req) {
                        //ToDo: Implement handling of the translation job response
                    }

                    /**
                     * Allows to notify the translations manager that there is a server
                     * adapter disconnected, so there will be no replies to the sent requests.
                     * @param uid the unique identifier of the adapter
                     */
                    inline void notify_adapter_disconnect(const trans_server_uid & uid) {
                        //ToDo: Implement cancellation of all the translation requests which have 
                        //      been sent but the response was not received yet.
                    }

                protected:

                    /**
                     * @see session_manager
                     */
                    virtual void session_is_closed(session_id_type session_id) {
                        //ToDo: Implement
                    }

                private:
                    //Stores the pointer to the server parameters
                    const balancer_parameters & m_params;
                    //Stores the reply sender functional
                    response_sender m_sender_func;
                };

            }
        }
    }
}

#endif /* TRANSLATION_MANAGER_HPP */

