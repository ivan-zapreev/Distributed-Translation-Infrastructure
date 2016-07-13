/* 
 * File:   balancer_server.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef BALANCER_SERVER_HPP
#define BALANCER_SERVER_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "balancer/balancer_parameters.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the balancer server class, this class is a singleton:
                 * Responsibilities:
                 *      Receives the supported languages requests 
                 *      Sends the supported languages responses.
                 *      Receives the translation requests
                 *      Places the received requests into dispatching queue 
                 *      Sends the translation responses.
                 */
                class balancer_server {
                public:

                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static void configure(const balancer_parameters & params) {
                        //ToDo: Implement
                    }

                    /**
                     * The main method to run in the server thread
                     */
                    static void run() {
                        //ToDo: Implement
                    }

                    /**
                     * Allows to stop the balancer server
                     */
                    static void stop() {
                        //ToDo: Implement
                    }

                private:

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    balancer_server() {
                    }

                };

            }
        }
    }
}

#endif /* BALANCER_SERVER_HPP */

