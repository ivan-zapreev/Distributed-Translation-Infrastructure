/* 
 * File:   bl_cmd_line_client.hpp
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
 * Created on May 24, 2018, 4:00 PM
 */

#ifndef BL_CMD_LINE_CLIENT_HPP
#define BL_CMD_LINE_CLIENT_HPP

#include "common/utils/cmd/cmd_line_client.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::cmd;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * Represents an abstract class to be realized by the
                 * balancer_server to become the command line client
                 * of the console.
                 */
                class bl_cmd_line_client: public cmd_line_client {
                public:

                    /**
                     * @see cmd_line_client
                     */
                    virtual void set_num_threads(const int32_t num_threads) override {
                        //There are two thread pools in the load balancer, therefor it has two other specific methods
                        THROW_NOT_SUPPORTED();
                    }

                    /**
                     * Allows to set a new number of incoming pool threads
                     * @param num_threads the new number of threads
                     */
                    virtual void set_num_inc_threads(const int32_t num_threads) = 0;

                    /**
                     * Allows to set a new number of outgoing pool threads
                     * @param num_threads the new number of threads
                     */
                    virtual void set_num_out_threads(const int32_t num_threads) = 0;

                };
            }
        }
    }
}

#endif /* BL_CMD_LINE_CLIENT_HPP */

