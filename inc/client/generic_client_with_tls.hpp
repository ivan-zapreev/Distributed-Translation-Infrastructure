/* 
 * File:   generic_client_with_tls.hpp
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

#ifndef GENERIC_CLIENT_WITH_TLS_HPP
#define GENERIC_CLIENT_WITH_TLS_HPP

#include "common/messaging/tls_mode.hpp"

#include "client/generic_client_base.hpp"

using namespace websocketpp;
using namespace websocketpp::lib::asio::ssl;

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

#if IS_TLS_SUPPORT

                /**
                 * This is an actual implementation of the generic client with TLS support
                 * 
                 * @param TLS_MODE the TLS mode 
                 */
                template<tls_mode_enum TLS_MODE>
                class generic_client_with_tls : public generic_client_base<websocketpp::config::asio_tls_client> {
                private:
                    //Define the context pointer type
                    typedef shared_ptr<context> context_ptr;

                protected:

                    static context_ptr on_tls_init(connection_hdl hdl) {
                        LOG_DEBUG << "Calling TLS initialization, mode: "
                                << TLS_MODE << " with handler: "
                                << hdl.lock().get() << END_LOG;

                        //Create the secure TLS context version 1 or higher
                        context_ptr ctx = make_shared<context>(context::tlsv12);
                        try {
                            switch (TLS_MODE) {
                                case tls_mode_enum::MOZILLA_MODERN:
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::no_sslv3 |
                                            context::no_tlsv1 |
                                            context::single_dh_use);
                                    break;
                                case tls_mode_enum::MOZILLA_INTERMEDIATE:
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::no_sslv3 |
                                            context::single_dh_use);
                                    break;
                                case tls_mode_enum::MOZILLA_OLD:
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::single_dh_use);
                                    break;
                                default:
                                    LOG_ERROR << "The TLS handshake mode is undefined!" << END_LOG;
                                    break;
                            }
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
                     */
                    generic_client_with_tls(const string & uri,
                            new_msg_notifier notify_new_msg,
                            conn_status_notifier notify_conn_close,
                            conn_status_notifier notify_conn_open)
                    : generic_client_base(uri, notify_new_msg,
                    notify_conn_close, notify_conn_open) {
                        //Register the TLS handshake handler
                        m_client.set_tls_init_handler(bind(&generic_client_with_tls::on_tls_init, this, _1));
                    }

                };
#else

                /**
                 * This is a stub implementation of the generic_client_with_tls
                 * for when TLS is not available. All of the virtual methods
                 * inherited from generic_client are throwing exceptions.
                 */
                class generic_client_with_tls : public generic_client {
                public:

                    /**
                     * The basic constructor
                     * @param uri the uri to connect to, must begin with wss://
                     * @param tls_mode the TLS mode
                     * @param notify_new_msg the function to call in case of a new incoming message
                     * @param notify_conn_close the function to call if the connection is closed
                     * @param notify_conn_open the function to call if the connection is open
                     */
                    generic_client_with_tls(const string & uri,
                            const tls_mode_enum tls_mode,
                            new_msg_notifier notify_new_msg,
                            conn_status_notifier notify_conn_close,
                            conn_status_notifier notify_conn_open)
                    : generic_client() {
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
#endif
            }
        }
    }
}

#endif /* GENERIC_CLIENT_WITH_TLS_HPP */

