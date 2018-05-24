/* 
 * File:   cmd_line_client.hpp
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
 * Created on May 24, 2018, 2:17 PM
 */

#ifndef CMD_LINE_CLIENT_HPP
#define CMD_LINE_CLIENT_HPP

namespace uva {
    namespace utils {
        namespace cmd {

            /**
             * Represents a common interface for all command line client classes.
             * A command line client is supposed, so some extend to be controller from the command line.
             */
            class cmd_line_client {
            public:
                /**
                 * Allows to report the runtime information about the client printed to console.
                 */
                virtual void report_run_time_info() = 0;

                /**
                 * Allows to request the client to stop its execution
                 */
                virtual void request_stop() = 0;

                /**
                 * Allows to set a new number of pool threads
                 * @param num_threads the new number of threads
                 */
                virtual void set_num_threads(const int32_t num_threads) = 0;
            };
        }
    }
}
#endif /* CMD_LINE_CLIENT_HPP */

