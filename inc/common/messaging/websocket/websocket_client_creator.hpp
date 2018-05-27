/* 
 * File:   websocket_client_creator.hpp
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
 * Created on May 27, 2018, 4:12 PM
 */

#ifndef WEBSOCKET_CLIENT_CREATOR_HPP
#define WEBSOCKET_CLIENT_CREATOR_HPP

#include "common/messaging/websocket/websocket_client.hpp"
#include "common/messaging/websocket/websocket_client_params.hpp"
#include "common/messaging/websocket/websocket_client_with_tls.hpp"
#include "common/messaging/websocket/websocket_client_without_tls.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * The class encapsulating the WebSocket creation function,
                         *  can be used as a base class for any class requiring
                         *  WebSocket client instantiation.
                         */
                        class websocket_client_creator {
                        protected:

                            /**
                             * Allows to create a new WebSocket client 
                             * @param m_params the WebSocket client parameters
                             * @param new_msg the new message callback function
                             * @param conn_closed the connection closed callback function
                             * @param conn_opened the connection opened callback function
                             */
                            static inline websocket_client * create_websocket_client(
                                    const websocket_client_params & params,
                                    websocket_client::new_msg_notifier new_msg,
                                    websocket_client::conn_status_notifier conn_closed,
                                    websocket_client::conn_status_notifier conn_opened) {

                                //Stores the pointer to the translation client
                                websocket_client * client;

                                //Create a new client
                                if (params.m_is_tls_client) {
                                    switch (params.m_tls_mode) {
                                        case tls_mode_enum::MOZILLA_OLD:
                                            client = new websocket_client_tls_old(
                                                    params.m_server_uri, new_msg,
                                                    conn_closed, conn_opened);
                                            break;
                                        case tls_mode_enum::MOZILLA_INTERMEDIATE:
                                            client = new websocket_client_tls_int(
                                                    params.m_server_uri, new_msg,
                                                    conn_closed, conn_opened);
                                            break;
                                        case tls_mode_enum::MOZILLA_MODERN:
                                            client = new websocket_client_tls_mod(
                                                    params.m_server_uri, new_msg,
                                                    conn_closed, conn_opened);
                                            break;
                                        default:
                                            THROW_EXCEPTION("The client is requested but the TLS mode is undefinedF!");
                                            break;
                                    }
                                } else {
                                    client = new websocket_client_no_tls(
                                            params.m_server_uri, new_msg,
                                            conn_closed, conn_opened);
                                }

                                return client;
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_CREATOR_HPP */

