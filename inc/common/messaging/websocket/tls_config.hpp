/* 
 * File:   tls_config.hpp
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
 * Created on May 27, 2018, 10:32 AM
 */

#ifndef TLS_CONFIG_HPP
#define TLS_CONFIG_HPP

#include <websocketpp/config/asio.hpp>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/websocket/tls_mode.hpp"

using namespace websocketpp;
using namespace websocketpp::lib::asio::ssl;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        //Define the context pointer type
                        typedef shared_ptr<context> context_ptr;

                        /**
                         * Allows to create a new secure TLS context
                         * @param TLS_MODE the selected TLS mode
                         * @param IS_SERVER_CTX true if a server context is created
                         * @param ciphers the ciphers selected to be used, or an empty string for system-default ones
                         * @return the new secure TLS context
                         */
                        template<tls_mode_enum TLS_MODE, bool IS_SERVER_CTX>
                        inline context_ptr create_tls_context(const string & ciphers) {
                            //Define the TLS context to be initialized
                            context_ptr ctx;

                            //Initialize based on the TLS mode
                            switch (TLS_MODE) {
                                case tls_mode_enum::MOZILLA_MODERN:
                                    ctx = make_shared<context>(context::tlsv12);
                                    if (IS_SERVER_CTX) {
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::no_sslv3 |
                                                context::no_tlsv1 |
                                                context::single_dh_use);
                                    }
                                    break;
                                case tls_mode_enum::MOZILLA_INTERMEDIATE:
                                    //WARNING:: Using tlsv1 causes TLS handshake
                                    //failure, therefore using tlsv12!
                                    ctx = make_shared<context>(context::tlsv12);
                                    if (IS_SERVER_CTX) {
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::no_sslv3 |
                                                context::single_dh_use);
                                    }
                                    break;
                                case tls_mode_enum::MOZILLA_OLD:
                                    ctx = make_shared<context>(context::sslv23);
                                    if (IS_SERVER_CTX) {
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::single_dh_use);
                                    }
                                    break;
                                default:
                                    THROW_EXCEPTION("The TLS handshake mode is undefined!");
                                    break;
                            }
                            
                            //Check if the ciphers are provided then set them into the list,
                            //if not set then the system-default ciphers will be used.
                            //WARNING: Setting the ciphers list, may cause TLS handshake failure!
                            string local_ciphers = ciphers;
                            if (!trim(local_ciphers).empty()) {
                                if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
                                    THROW_EXCEPTION(string("None of the TLS ciphers could be selected out of: ") + ciphers);
                                }
                            }

                            return ctx;
                        }
                    }
                }
            }
        }
    }
}

#endif /* TLS_CONFIG_HPP */

