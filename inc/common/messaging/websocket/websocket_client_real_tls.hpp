/* 
 * File:   websocket_client_real_tls.hpp
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
 * Created on May 27, 2018, 10:17 AM
 */

#ifndef WEBSOCKET_CLIENT_REAL_TLS_HPP
#define WEBSOCKET_CLIENT_REAL_TLS_HPP

#include "common/messaging/websocket/websocket_client_base.hpp"

#include "common/messaging/websocket/tls_mode.hpp"
#include "common/messaging/websocket/tls_config.hpp"

using namespace websocketpp;
using namespace websocketpp::lib::asio::ssl;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This is an actual implementation of the generic client with TLS support
                         * 
                         * @param TLS_MODE the TLS mode 
                         */
                        template<tls_mode_enum TLS_MODE>
                        class websocket_client_with_tls
                        : public websocket_client_base<websocketpp::config::asio_tls_client> {
                        protected:

                            static context_ptr on_tls_init(connection_hdl hdl) {
                                LOG_DEBUG << "Calling TLS initialization, mode: "
                                        << TLS_MODE << " with handler: "
                                        << hdl.lock().get() << END_LOG;

                                //Define the TLS context
                                context_ptr ctx = make_shared<context>(context::tls);

                                //Configure the TLS context
                                try {
                                    //Create TLS context depending on its mode
                                    ctx = create_tls_context<TLS_MODE, false>();
                                } catch (std::exception& e) {
                                    LOG_ERROR << "An unexpected exception during "
                                            << "the TLS handshake: " << e.what() << END_LOG;
                                }

                                return ctx;
                            }
                        public:

                            /**
                             * The basic constructor
                             * @param uri the uri to connect to, must begin with wss://
                             * @param notify_new_msg the function to call in case of a new incoming message
                             * @param notify_conn_close the function to call if the connection is closed
                             * @param notify_conn_open the function to call if the connection is open
                             * @param is_warn_failed if true then a warning is issued once the connection fails
                             */
                            websocket_client_with_tls(const string & uri,
                                    new_msg_notifier notify_new_msg,
                                    conn_status_notifier notify_conn_close,
                                    conn_status_notifier notify_conn_open,
                                    const bool is_warn_failed)
                            : websocket_client_base(uri, notify_new_msg,
                            notify_conn_close, notify_conn_open, is_warn_failed) {
                                //Register the TLS handshake handler
                                m_client.set_tls_init_handler(
                                        bind(&websocket_client_with_tls<TLS_MODE>::on_tls_init, _1));
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_REAL_TLS_HPP */

