/* 
 * File:   websocket_client_params.cpp
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
 * Created on May 27, 2018, 12:22 PM
 */

#include "common/messaging/websocket/websocket_client_params.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {
                        const string websocket_client_params::WC_SERVER_URI_PARAM_NAME = "server_uri";

                        const string websocket_client_params::WC_IS_TLS_CLIENT_PARAM_NAME = "is_tls_client";
                        
                        const string websocket_client_params::WC_SERVER_URI_REG_EXP_STR = "^((ws)|(wss))\\:\\/\\/.*:\\d+";
                        const string websocket_client_params::WC_IS_TLS_CLIENT_REG_EXP_STR = "^wss\\:\\/\\/.*";

                        const string websocket_client_params::WC_TLS_MODE_PARAM_NAME = "tls_mode";
                        const string websocket_client_params::WC_TLS_MODE_REG_EXP_STR = string("(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_OLD) + string(")|(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_INTERMEDIATE) + string(")|(") +
                        tls_val_to_str(tls_mode_enum::MOZILLA_MODERN) + string(")");
                    }
                }
            }
        }
    }
}