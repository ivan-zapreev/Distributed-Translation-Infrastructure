/* 
 * File:   load_balancer.hpp
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
 * Created on July 7, 2016, 12:08 PM
 */

#ifndef LOAD_BALANCER_HPP
#define LOAD_BALANCER_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                
                /**
                 * This is the main load balancer application class:
                 * Responsibilities:
                 *      Reads the configuration file
                 *      Configures the translation_servers_manager
                 *      Configures the balancer_server
                 *      Starts the balancer_server
                 *      Starts the balancer_console
                 */
                class load_balancer {
                };
                
            }
        }
    }
}

#endif /* LOAD_BALANCER_HPP */

