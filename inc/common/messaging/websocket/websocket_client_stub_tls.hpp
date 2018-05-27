/* 
 * File:   websocket_client_stub_tls.hpp
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
 * Created on May 27, 2018, 10:19 AM
 */

#ifndef WEBSOCKET_CLIENT_STUB_TLS_HPP
#define WEBSOCKET_CLIENT_STUB_TLS_HPP

#include "client/websocket_client.hpp"
#include "common/messaging/tls_mode.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This is a stub implementation of the generic_client_with_tls
                         * for when TLS is not available. All of the virtual methods
                         * inherited from generic_client are throwing exceptions.
                         */
                        class websocket_client_stub_tls : public websocket_client {
                        public:

                            /**
                             * The basic constructor
                             * @param uri the uri to connect to, must begin with wss://
                             * @param tls_mode the TLS mode
                             * @param notify_new_msg the function to call in case of a new incoming message
                             * @param notify_conn_close the function to call if the connection is closed
                             * @param notify_conn_open the function to call if the connection is open
                             */
                            websocket_client_stub_tls(const string & uri,
                                    const tls_mode_enum tls_mode,
                                    new_msg_notifier notify_new_msg,
                                    conn_status_notifier notify_conn_close,
                                    conn_status_notifier notify_conn_open)
                            : websocket_client() {
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual void connect_nb() override {
                                THROW_NOT_SUPPORTED();
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual bool connect() override {
                                THROW_NOT_SUPPORTED();
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual void disconnect() override {
                                THROW_NOT_SUPPORTED();
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual void send(const msg_base * message) override {
                                THROW_NOT_SUPPORTED();
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual const string get_uri() const override {
                                THROW_NOT_SUPPORTED();
                            }

                            /**
                             * @see generic_client 
                             */
                            virtual bool is_connected() const override {
                                THROW_NOT_SUPPORTED();
                            }
                        };

                        //Make the dummy client the tls client
                        typedef websocket_client_stub_tls generic_client_with_tls;
                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_STUB_TLS_HPP */

