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

#include "common/messaging/websocket/websocket_server_params.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {
                        const string websocket_server_params::WS_SERVER_PORT_PARAM_NAME = "server_port";

                        const string websocket_server_params::WS_IS_TLS_SERVER_PARAM_NAME = "is_tls_server";

                        const string websocket_server_params::WS_TLS_MODE_PARAM_NAME = "tls_mode";
                        const string websocket_server_params::WS_TLS_MODE_REG_EXP_STR = string("(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_UNDEFINED) + string(")|(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_OLD) + string(")|(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_INTERMEDIATE) + string(")|(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_MODERN) + string(")");
                        
                        const string websocket_server_params::WS_TLS_CIPHERS_PARAM_NAME = "tls_ciphers";

                        const string websocket_server_params::WS_TLS_CRT_FILE_PARAM_NAME = "tls_crt_file";
                        const string websocket_server_params::WS_TLS_CRT_FILE_REG_EXP_STR = ".*\\.((crt)|(pem))$";

                        const string websocket_server_params::WS_TLS_KEY_FILE_PARAM_NAME = "tls_key_file";
                        const string websocket_server_params::WS_TLS_KEY_FILE_REG_EXP_STR = ".*\\.((key)|(pem))$";

                        const string websocket_server_params::WS_TLS_DH_FILE_PARAM_NAME = "tls_tmp_dh_file";
                        const string websocket_server_params::WS_TLS_DH_FILE_REG_EXP_STR = ".*\\.pem$";
                    }
                }
            }
        }
    }
}