/* 
 * File:   websocket_parameterss.cpp
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
 * Created on May 24, 2018, 11:19 PM
 */

#include "common/messaging/websocket_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    const string websocket_parameters::SE_SERVER_PORT_PARAM_NAME = "server_port";
                    const string websocket_parameters::SE_IS_TLS_SERVER_PARAM_NAME = "is_tls_server";
                    const string websocket_parameters::SE_TLS_CERT_FILE_PARAM_NAME = "cert_file";
                    const string websocket_parameters::SE_TLS_KEY_FILE_PARAM_NAME = "priv_key_file";
                    const string websocket_parameters::SE_TLS_DH_FILE_PARAM_NAME = "tmp_dh_file";
                    const string websocket_parameters::SE_TLS_CERT_FILE_REG_EXP_STR = ".*\\.((crt)|(pem))$";
                    const string websocket_parameters::SE_TLS_KEY_FILE_REG_EXP_STR = ".*\\.((key)|(pem))$";
                    const string websocket_parameters::SE_TLS_DH_FILE_REG_EXP_STR = ".*\\.pem$";
                }
            }
        }
    }
}