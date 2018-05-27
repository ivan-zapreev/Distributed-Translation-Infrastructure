/* 
 * File:   websocket_client_with_tls.hpp
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
 * Created on May 26, 2018, 10:32 PM
 */

#ifndef WEBSOCKET_CLIENT_WITH_TLS_HPP
#define WEBSOCKET_CLIENT_WITH_TLS_HPP

#include "common/messaging/websocket/websocket_client.hpp"
#if IS_TLS_SUPPORT
#include "common/messaging/websocket/websocket_client_real_tls.hpp"
#else
#include "common/messaging/websocket/websocket_client_stub_tls.hpp"
#endif

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        //Make the dummy client the tls client
                        typedef websocket_client_with_tls<tls_mode_enum::MOZILLA_OLD> websocket_client_tls_old;
                        typedef websocket_client_with_tls<tls_mode_enum::MOZILLA_INTERMEDIATE> websocket_client_tls_int;
                        typedef websocket_client_with_tls<tls_mode_enum::MOZILLA_MODERN> websocket_client_tls_mod;

                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_WITH_TLS_HPP */

