/* 
 * File:   server_tls_handshake.hpp
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
 * Created on May 24, 2018, 10:04 PM
 */

#ifndef SERVER_TLS_HANDSHAKE_HPP
#define SERVER_TLS_HANDSHAKE_HPP

#include <string>
#include <iostream>
#include <functional>
#include <fstream>
#include <streambuf>

#define ASIO_STANDALONE
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio.hpp>

#include "common/utils/text/string_utils.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/websocket/tls_mode.hpp"
#include "common/messaging/websocket/tls_config.hpp"
#include "common/messaging/websocket/websocket_server_params.hpp"

using namespace std;
using namespace std::placeholders;

using namespace websocketpp;
using namespace websocketpp::lib::asio::ssl;

using namespace uva::utils::text;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using websocketpp::lib::asio::const_buffer;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This template class provides server's real TLS handshake method
                         * 
                         * @param TLS_MODE the TLS mode
                         */
                        template<tls_mode_enum TLS_MODE>
                        class server_hs_with_tls {
                        private:
                            //Stores the server certificate string
                            const string m_server_crt_str;
                            //Stores the server certificate buffer
                            const const_buffer m_server_crt_buf;

                            //Stores the server private key string
                            const string m_server_key_str;
                            //Stores the server private key buffer
                            const const_buffer m_server_key_buf;

                            //Stores the server dh pem string
                            string m_tmp_dh_pem_str;
                            //Stores the server dh pem buffer
                            const_buffer m_tmp_dh_pem_buf;

                            /**
                             * Allows to read data from file into string
                             * @param file_name the file name to read the data from
                             * @return data_str the data string read from file
                             */
                            string red_data_from_file(const string & file_name) {
                                string data_str = "";
                                ifstream file(file_name);
                                if (file.is_open()) {
                                    data_str.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
                                } else {
                                    LOG_ERROR << "Failed to reading TLS related data from file: " << file_name << END_LOG;
                                }
                                return data_str;
                            }

                        public:
                            //Define the server type
                            typedef websocketpp::server<websocketpp::config::asio_tls> server_type;

                            /**
                             * The basic constructor
                             * @param params the WebSocket server parameters
                             * @param server the WebSocket server object
                             */
                            server_hs_with_tls(
                                    const websocket_server_params & params, server_type & server) :
                            m_server_crt_str(red_data_from_file(params.m_tls_crt_file)),
                            m_server_crt_buf(m_server_crt_str.data(), m_server_crt_str.size()),
                            m_server_key_str(red_data_from_file(params.m_tls_key_file)),
                            m_server_key_buf(m_server_key_str.data(), m_server_key_str.size()),
                            m_tmp_dh_pem_str(red_data_from_file(params.m_tls_dh_file)),
                            m_tmp_dh_pem_buf(m_tmp_dh_pem_str.data(), m_tmp_dh_pem_str.size()) {
                                //Bind the TLS initialization handler to the provided server
                                server.set_tls_init_handler(
                                        bind(&server_hs_with_tls<TLS_MODE>::on_tls_init, this, params.m_ciphers, _1));
                            }

                            /**
                             * The basic destructor
                             */
                            virtual ~server_hs_with_tls() {
                                //Nothing to be done here.
                            }

                            /**
                             * Is used to initialize the TLS context
                             * @param ciphers the list of allowed ciphers or an empty list for using defaults
                             * @param hdl the connection handler to be used
                             * @return the created TLS context
                             */
                            context_ptr on_tls_init(
                                    const string & ciphers, connection_hdl hdl) {
                                LOG_DEBUG << "Calling TLS initialization, mode: "
                                        << TLS_MODE << " with handler: "
                                        << hdl.lock().get() << END_LOG;

                                //Define the TLS context, default initialization for the case of an error
                                context_ptr ctx = make_shared<context>(context::tls);

                                //Configure the TLS context
                                try {
                                    //Create TLS context depending on its mode
                                    ctx = create_tls_context<TLS_MODE, true>(ciphers);

                                    //Set the server's certificate
                                    ctx->use_certificate_chain(m_server_crt_buf);
                                    //Set the server's private key
                                    ctx->use_private_key(m_server_key_buf, context::pem);
                                    //Set the DH parameters, these are optional
                                    ctx->use_tmp_dh(m_tmp_dh_pem_buf);
                                } catch (std::exception& e) {
                                    LOG_ERROR << "An unexpected exception "
                                            << "during TLS initialization: "
                                            << e.what() << END_LOG;
                                }

                                return ctx;
                            }
                        };

                        typedef server_hs_with_tls<tls_mode_enum::MOZILLA_OLD> server_hs_with_tls_old;
                        typedef server_hs_with_tls<tls_mode_enum::MOZILLA_INTERMEDIATE> server_hs_with_tls_int;
                        typedef server_hs_with_tls<tls_mode_enum::MOZILLA_MODERN> server_hs_with_tls_mod;
                    }
                }
            }
        }
    }
}

#endif /* SERVER_TLS_HANDSHAKE_HPP */

