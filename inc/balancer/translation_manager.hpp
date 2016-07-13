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

#include "client/messaging/trans_job_resp_in.hpp"

#include "balancer/balancer_parameters.hpp"
#include "balancer/translation_job.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::client::messaging;

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
                class translation_manager {
                public:

                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static inline void configure(const balancer_parameters & params) {
                        //Save the pointer to the parameters
                        m_params = &params;
                    }

                    /**
                     * The main method to start the translation manager
                     */
                    static inline void start() {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to stop the translation manager
                     */
                    static inline void stop() {
                        //ToDo: Implement
                    }
                    
                    /**
                     * Reports the run-time information
                     */
                    static inline void report_run_time_info() {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to process the server translation job response message
                     * @param trans_job_resp a pointer to the translation job response data, not NULL
                     */
                    static inline void set_job_response(trans_job_resp_in * trans_job_resp) {
                        //ToDo: Implement handling of the translation job response
                    }

                private:
                    //Stores the pointer to the server parameters
                    static const balancer_parameters * m_params;

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    translation_manager() {
                    }
                };

            }
        }
    }
}

#endif /* TRANSLATION_MANAGER_HPP */

