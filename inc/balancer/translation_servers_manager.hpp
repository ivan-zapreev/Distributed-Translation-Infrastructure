/* 
 * File:   translation_servers_manager.hpp
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

#ifndef TRANSLATION_SERVERS_MANAGER_HPP
#define TRANSLATION_SERVERS_MANAGER_HPP

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
                 * This is the translation servers manager class:
                 * Responsibilities:
                 *      Keeps track of the online translation servers
                 *      Keeps track of languages supported by the servers
                 *      Keeps track of the known load on the servers
                 *      Advises translation server for a translation request
                 */
                class translation_servers_manager {
                public:
                    
                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static void configure(const balancer_parameters & params) {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * The main method to start the translation servers manager
                     */
                    static void start() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to stop the translation servers manager
                     */
                    static void stop() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                private:

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    translation_servers_manager() {
                    }
                };
            }
        }
    }
}
#endif /* TRANSLATION_SERVERS_MANAGER_HPP */

