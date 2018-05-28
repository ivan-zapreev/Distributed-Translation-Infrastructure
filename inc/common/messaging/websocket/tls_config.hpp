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
                         * @return the new secure TLS context
                         */
                        template<tls_mode_enum TLS_MODE, bool IS_SERVER_CTX>
                        inline context_ptr create_tls_context() {
                            //Define the TLS context to be initialized
                            context_ptr ctx;

                            string ciphers;
                            switch (TLS_MODE) {
                                case tls_mode_enum::MOZILLA_MODERN:
                                    ctx = make_shared<context>(IS_SERVER_CTX ? context::tlsv12_server : context::tlsv12_client);
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::no_sslv3 |
                                            context::no_tlsv1 |
                                            context::single_dh_use);
                                    ciphers = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256";
                                    break;
                                case tls_mode_enum::MOZILLA_INTERMEDIATE:
                                    ctx = make_shared<context>(IS_SERVER_CTX ? context::tlsv1_server : context::tlsv1_client);
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::no_sslv3 |
                                            context::single_dh_use);
                                    ciphers = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS";
                                    break;
                                case tls_mode_enum::MOZILLA_OLD:
                                    ctx = make_shared<context>(IS_SERVER_CTX ? context::sslv23_server : context::sslv23_client);
                                    ctx->set_options(
                                            context::default_workarounds |
                                            context::no_sslv2 |
                                            context::single_dh_use);
                                    ciphers = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:ECDHE-RSA-DES-CBC3-SHA:ECDHE-ECDSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:DES-CBC3-SHA:HIGH:SEED:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!RSAPSK:!aDH:!aECDH:!EDH-DSS-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA:!SRP";
                                    break;
                                default:
                                    THROW_EXCEPTION("The TLS handshake mode is undefined!");
                                    break;
                            }

                            //Set the cipher lists
                            if (IS_SERVER_CTX) {
                                if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) == 0) {
                                    LOG_ERROR << "None of the TLS ciphers could be selected out of: " << ciphers << END_LOG;
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

