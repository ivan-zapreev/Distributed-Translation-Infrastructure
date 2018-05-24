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

#ifndef SERVER_TLS_HANDSHAKE_HH
#define SERVER_TLS_HANDSHAKE_HH

#include <string>
#include <iostream>
#include <functional>
#include <fstream>
#include <streambuf>

#include <websocketpp/config/asio.hpp>

#include "common/utils/text/string_utils.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace std;
using namespace std::placeholders;

using namespace websocketpp;
using namespace websocketpp::lib;
using namespace websocketpp::lib::asio::ssl;

using namespace uva::utils::text;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /* This enumeration defined the TLS mode
                     * See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
                     * the TLS modes. The code below demonstrates how to implement both the modern
                     */
                    enum tls_mode {
                        MOZILLA_UNDEFINED = 0,
                        MOZILLA_OLD = MOZILLA_UNDEFINED + 1,
                        MOZILLA_INTERMEDIATE = MOZILLA_OLD + 1,
                        MOZILLA_MODERN = MOZILLA_INTERMEDIATE + 1
                    };

                    /**
                     * Allows to convert the MOZILLA TLS mode name
                     * (old, intermediate, modern) into a the corresponding TLS mode.
                     * @param tls_str the TLS name string
                     * @return the corresponding TLS mode or MOZILLA_UNDEFINED if not recognized
                     */
                    inline tls_mode tls_str_to_val(string tls_str) {
                        //Turn into trimmed low-case string
                        to_lower(trim(tls_str));
                        //Compare with constant literals
                        if (tls_str.compare("old") == 0) {
                            return tls_mode::MOZILLA_OLD;
                        } else {
                            if (tls_str.compare("intermediate") == 0) {
                                return tls_mode::MOZILLA_INTERMEDIATE;
                            } else {
                                if (tls_str.compare("modern") == 0) {
                                    return tls_mode::MOZILLA_MODERN;
                                } else {
                                    return tls_mode::MOZILLA_UNDEFINED;
                                }
                            }
                        }
                    }
                    
                    /**
                     * This template class provides server's TLS handshake method
                     */
                    template<tls_mode MODE>
                    class server_tls_handshake {
                    private:
                        //Define the context pointer type
                        typedef shared_ptr<context> context_ptr;

                        //Stores the server certificate string
                        string m_server_crt_str;
                        //Stores the server key string
                        string m_server_key_str;
                        //Stores the server dh pem string
                        string m_tmp_dh_pem_str;

                        /**
                         * Allows to read data from file into string
                         * @param file_name the file name to read the data from
                         * @param data_str the data string read from file
                         */
                        void red_data_from_file(const string & file_name, string & data_str) {
                            ifstream file(file_name);
                            if (file.is_open()) {
                                data_str.assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
                            } else {
                                LOG_ERROR << "Failed to reading TLS related data from file: " << file_name << END_LOG;
                            }
                        }

                    public:

                        /**
                         * The basic constructor
                         * @param server_crt_file_name the name of the server certificate file
                         * @param server_key_file_name the name of the server key file
                         * @param tmp_dh_pem_name the name of the server's temporary DH pem file
                         */
                        server_tls_handshake(
                                const string & server_crt_file_name,
                                const string & server_key_file_name,
                                const string & tmp_dh_pem_name) {
                            red_data_from_file(server_crt_file_name, m_server_crt_str);
                            red_data_from_file(server_key_file_name, m_server_key_str);
                            red_data_from_file(tmp_dh_pem_name, m_tmp_dh_pem_str);
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~server_tls_handshake() {
                            //Nothing to be done here.
                        }

                        /**
                         * Allows to perform the TLS initialization
                         */
                        context_ptr on_tls_init(connection_hdl hdl) {
                            namespace asio = websocketpp::lib::asio;

                            LOG_DEBUG << "Calling TLS initialization, mode: "
                                    << MODE << " with handler: "
                                    << hdl.lock().get() << END_LOG;

                            //Create the secure SSL v2.3 context 
                            context_ptr ctx = make_shared<context>(context::sslv23);

                            try {
                                string ciphers;
                                switch (MODE) {
                                    case tls_mode::MOZILLA_MODERN:
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::no_sslv3 |
                                                context::no_tlsv1 |
                                                context::single_dh_use);
                                        ciphers = "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256";
                                        break;
                                    case tls_mode::MOZILLA_INTERMEDIATE:
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::no_sslv3 |
                                                context::single_dh_use);
                                        ciphers = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS";
                                        break;
                                    case tls_mode::MOZILLA_OLD:
                                        ctx->set_options(
                                                context::default_workarounds |
                                                context::no_sslv2 |
                                                context::single_dh_use);
                                        ciphers = "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:ECDHE-RSA-DES-CBC3-SHA:ECDHE-ECDSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:DES-CBC3-SHA:HIGH:SEED:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!RSAPSK:!aDH:!aECDH:!EDH-DSS-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA:!SRP";
                                        break;
                                    default:
                                        LOG_ERROR << "The TLS handshake mode is undefined!" << END_LOG;
                                        break;
                                }

                                //Set the certificates data
                                ctx->use_certificate_chain(m_server_crt_str.data());
                                ctx->use_private_key_file(m_server_key_str.data(), context::pem);
                                ctx->use_tmp_dh_file(m_tmp_dh_pem_str.data());

                                //Set the cipher lists
                                if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) == 0) {
                                    LOG_ERROR << "None of the TLS ciphers could be selected out of: " << ciphers << END_LOG;
                                }
                            } catch (std::exception& e) {
                                LOG_ERROR << "An unexpected exception during "
                                        << "the TLS handshake: " << e.what() << END_LOG;
                            }

                            return ctx;
                        }
                    };
                }
            }
        }
    }
}

#endif /* SERVER_TLS_HANDSHAKE_HH */

