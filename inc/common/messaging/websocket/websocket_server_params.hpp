/* 
 * File:   websocket_server_params.hpp
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
 * Created on May 24, 2018, 11:19 PM
 */

#ifndef WEBSOCKET_SERVER_PARAMS_HPP
#define WEBSOCKET_SERVER_PARAMS_HPP

#include <string>
#include <fstream>
#include <regex>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"

#include "common/messaging/websocket/tls_mode.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::text;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This is the storage for websocket server parameters:
                         * Responsibilities:
                         *      Store the run-time parameters of the websocket server
                         */
                        struct websocket_server_params_struct {
                        private:
                            //Stores the server TLS mode regular expression
                            static const string WS_TLS_MODE_REG_EXP_STR;
                            //The server TLS certificate file name regular expression
                            static const string WS_TLS_CRT_FILE_REG_EXP_STR;
                            //The server TLS private key file name regular expression
                            static const string WS_TLS_KEY_FILE_REG_EXP_STR;
                            //The TLS temporary DH pem file name regular expression
                            static const string WS_TLS_DH_FILE_REG_EXP_STR;

                        public:
                            //Stores the server port parameter name
                            static const string WS_SERVER_PORT_PARAM_NAME;

                            //Stores the server TLS support parameter name
                            static const string WS_IS_TLS_SERVER_PARAM_NAME;

                            //Stores the server TLS mode parameter name
                            static const string WS_TLS_MODE_PARAM_NAME;

                            //Stores the server TLS ciphers parameter name
                            static const string WS_TLS_CIPHERS_PARAM_NAME;

                            //Stores the server TLS certificate parameter name
                            static const string WS_TLS_CRT_FILE_PARAM_NAME;

                            //Stores the server TLS private key parameter name
                            static const string WS_TLS_KEY_FILE_PARAM_NAME;

                            //Stores the TLS temporary DH pem parameter name
                            static const string WS_TLS_DH_FILE_PARAM_NAME;

                            //The port to listen to
                            uint16_t m_server_port;
                            //The flag indicating whether the TLS server is running
                            bool m_is_tls_server;
                            //Stores the TLS mode string name for the case of the TLS server
                            string m_tls_mode_name;
                            //Stores the TLS mode for the case of the TLS server
                            tls_mode_enum m_tls_mode;
                            //Stores the server's certificate file name
                            string m_tls_crt_file;
                            //Stores the server's private key file name
                            string m_tls_key_file;
                            //Stores the server's DH pem file name
                            string m_tls_dh_file;
                            //Stores the client's ciphers, or an empty string for defaults
                            string m_tls_ciphers;

                            /**
                             * Allows to check if the file exists
                             * @param file_name the file name to be checked
                             * @return true if the file exists, otherwise false
                             */
                            bool file_exists(const string& file_name) {
                                ifstream file(file_name);
                                return file.good();
                            }

                            /**
                             * Allows to finalize the parameters after loading.
                             */
                            virtual void finalize() {
#if (!defined(WITH_TLS) || !WITH_TLS)
                                if (m_is_tls_server) {
                                    LOG_WARNING << "The value of the "
                                            << WS_IS_TLS_SERVER_PARAM_NAME
                                            << " is set to TRUE but the server is not"
                                            << " built with TLS support, re-setting"
                                            << " to FALSE!" << END_LOG;
                                    m_is_tls_server = false;
                                }
#else
                                if (m_is_tls_server) {
                                    const regex tls_mode_name_regexp(WS_TLS_MODE_REG_EXP_STR);
                                    ASSERT_CONDITION_THROW(
                                            (!regex_match(m_tls_mode_name, tls_mode_name_regexp)),
                                            string("The server TLS mode name: '") +
                                            m_tls_mode_name + string("' does not match ") +
                                            string(" the allowed pattern: '") +
                                            WS_TLS_MODE_REG_EXP_STR + string("'!"));
                                    //Convert the TLS mode name into its value
                                    m_tls_mode = tls_str_to_val(m_tls_mode_name);

                                    //Check if the mode is set to undefined, this is not allowed
                                    ASSERT_CONDITION_THROW((m_tls_mode == tls_mode_enum::MOZILLA_UNDEFINED),
                                            string("The TLS server is requested but the TLS ") +
                                            string("mode is set to: '") + m_tls_mode_name + string("'"));

                                    //Check on the certificate file
                                    const regex server_crt_regexp(WS_TLS_CRT_FILE_REG_EXP_STR);
                                    ASSERT_CONDITION_THROW(
                                            (!regex_match(m_tls_crt_file, server_crt_regexp)),
                                            string("The server TLS certificate file name: ") +
                                            m_tls_crt_file + string(" does not match ") +
                                            string(" the allowed extensions: '") +
                                            WS_TLS_CRT_FILE_REG_EXP_STR + string("'!"));
                                    ASSERT_CONDITION_THROW((!file_exists(m_tls_crt_file)),
                                            string("The server TLS certificate file: ") +
                                            m_tls_crt_file + string(" does not exist! "));

                                    //Check on the private key file
                                    const regex priv_key_regexp(WS_TLS_KEY_FILE_REG_EXP_STR);
                                    ASSERT_CONDITION_THROW(
                                            (!regex_match(m_tls_key_file, priv_key_regexp)),
                                            string("The server TLS private key file: ") +
                                            m_tls_key_file + string(" does not match ") +
                                            string(" the allowed extensions: '") +
                                            WS_TLS_KEY_FILE_REG_EXP_STR + string("'!"));
                                    ASSERT_CONDITION_THROW((!file_exists(m_tls_key_file)),
                                            string("The server TLS private key file: ") +
                                            m_tls_key_file + string(" does not exist! "));

                                    //Check on the DH parameters file
                                    const regex tmp_dh_pem_regexp(WS_TLS_DH_FILE_REG_EXP_STR);
                                    ASSERT_CONDITION_THROW(
                                            (!regex_match(m_tls_dh_file, tmp_dh_pem_regexp)),
                                            string("The server TLS tmp DH pem file: ") +
                                            m_tls_dh_file + string(" does not match ") +
                                            string(" the allowed extensions: '") +
                                            WS_TLS_DH_FILE_REG_EXP_STR + string("'!"));
                                    ASSERT_CONDITION_THROW((!file_exists(m_tls_dh_file)),
                                            string("The server TLS tmp DH pem file: ") +
                                            m_tls_dh_file + string(" does not exist! "));

                                    //Check on the ciphers
                                    m_tls_ciphers = trim(m_tls_ciphers);
                                    if (!m_tls_ciphers.empty()) {
                                        LOG_WARNING << "The WebSocker server is requested "
                                                << "to use custom ciphers: '" << m_tls_ciphers
                                                << "'" << ", this may cause 'TLS handshake "
                                                << "failure', please we warned!" << END_LOG;
                                    }
                                } else {
                                    m_tls_mode = tls_mode_enum::MOZILLA_UNDEFINED;
                                }
#endif
                            }
                        };

                        //Typedef the structure
                        typedef websocket_server_params_struct websocket_server_params;

                        /**
                         * Allows to output the parameters object to the stream
                         * @param stream the stream to output into
                         * @param params the parameters object
                         * @return the stream that we output into
                         */
                        static inline std::ostream& operator<<(
                                std::ostream& stream, const websocket_server_params & params) {
                            //Dump the parameters
                            stream << "WebSocket server parameters: {"
                                    << websocket_server_params::WS_SERVER_PORT_PARAM_NAME
                                    << " = " << params.m_server_port
                                    << ", " << websocket_server_params::WS_IS_TLS_SERVER_PARAM_NAME
                                    << " = ";
                            if (params.m_is_tls_server) {
                                stream << "true"
                                        << ", " << websocket_server_params::WS_TLS_MODE_PARAM_NAME
                                        << " = " << tls_val_to_str(params.m_tls_mode)
                                        << ", " << websocket_server_params::WS_TLS_CRT_FILE_PARAM_NAME
                                        << " = " << params.m_tls_crt_file
                                        << ", " << websocket_server_params::WS_TLS_KEY_FILE_PARAM_NAME
                                        << " = " << params.m_tls_key_file
                                        << ", " << websocket_server_params::WS_TLS_DH_FILE_PARAM_NAME
                                        << " = " << params.m_tls_dh_file;
                                if (!params.m_tls_ciphers.empty()) {
                                    stream << ", " << websocket_server_params::WS_TLS_CIPHERS_PARAM_NAME
                                            << " = " << params.m_tls_ciphers;
                                }
                            } else {
                                stream << "false";
                            }
                            return stream << "}";
                        }
                    }
                }
            }
        }
    }
}

#endif /* WEBSOCKET_SERVER_PARAMS_HPP */

