/* 
 * File:   balancer_parameters.cpp
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
 * Created on July 8, 2016, 10:36 AM
 */

#include "balancer/balancer_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                const string trans_server_params::TC_ADDRESS_PARAM_NAME = "address";
                const string trans_server_params::TC_PORT_PARAM_NAME = "port";
                const string trans_server_params::TC_LOAD_WEIGHT_PARAM_NAME = "load_weight";

                const string balancer_parameters::SE_CONFIG_SECTION_NAME = "Server Options";
                const string balancer_parameters::SE_SERVER_PORT_PARAM_NAME = "server_port";
                const string balancer_parameters::SE_NUM_REQ_THREADS_PARAM_NAME = "num_req_threads";
                const string balancer_parameters::SE_NUM_RESP_THREADS_PARAM_NAME = "num_resp_threads";
                const string balancer_parameters::SE_TRANSLATION_SERVER_NAMES_PARAM_NAME = "translation_server_names";
                const string balancer_parameters::SC_RECONNECT_TIME_OUT_PARAM_NAME = "reconnect_time_out";
                const string balancer_parameters::TRANS_SERV_NAMES_DELIMITER_STR = "|";
            }
        }
    }
}