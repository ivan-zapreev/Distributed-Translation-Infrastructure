/* 
 * File:   server_parameters.cpp
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
 * Created on May 17, 2016, 12:15 AM
 */

#include "server/server_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                const string server_parameters::SE_CONFIG_SECTION_NAME = "Server Options";
                const string server_parameters::SE_SERVER_PORT_PARAM_NAME = "server_port";
                const string server_parameters::SE_IS_TLS_SERVER_PARAM_NAME = "is_tls_server";
                const string server_parameters::SE_NUM_THREADS_PARAM_NAME = "num_threads";
                const string server_parameters::SE_SOURCE_LANG_PARAM_NAME = "source_lang";
                const string server_parameters::SE_TARGET_LANG_PARAM_NAME = "target_lang";
            }
        }
    }
}