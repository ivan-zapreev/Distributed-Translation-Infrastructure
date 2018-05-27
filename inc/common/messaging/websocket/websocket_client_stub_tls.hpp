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

#include "common/messaging/websocket/websocket_client.hpp"
#include "common/messaging/websocket/tls_mode.hpp"

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
                         * 
                         * @param TLS_MODE the TLS mode 
                         */
                        template<tls_mode_enum TLS_MODE>
                        class websocket_client_with_tls
                        : public websocket_client {
                        public:

                            /**
                             * The basic constructor
                             * @param uri NOT USED as it is a STUB
                             * @param msg NOT USED as it is a STUB
                             * @param close NOT USED as it is a STUB
                             * @param open NOT USED as it is a STUB
                             */
                            websocket_client_with_tls(
                                    const string &, new_msg_notifier,
                                    conn_status_notifier, conn_status_notifier)
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
                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_CLIENT_STUB_TLS_HPP */

