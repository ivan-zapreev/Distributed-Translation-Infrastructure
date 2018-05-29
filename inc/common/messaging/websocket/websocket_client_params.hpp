/* 
 * File:   websocket_client_params.hpp
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
 * Created on May 27, 2018, 12:14 PM
 */

#ifndef WEBSOCKET_CLIENT_PARAMS_HPP
#define WEBSOCKET_CLIENT_PARAMS_HPP

#include <string>
#include <regex>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/messaging/websocket/tls_mode.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /**
                         * This is the storage for websocket client parameters:
                         * Responsibilities:
                         *      Store the run-time parameters of the websocket client
                         */
                        struct websocket_client_params_struct {
                        private:
                            //Stores the server URI regular expression
                            static const string WC_SERVER_URI_REG_EXP_STR;
                            //Stores the server TLS support URI detection
                            //regular expression string
                            static const string WC_IS_TLS_CLIENT_REG_EXP_STR;
                            //Stores the server TLS mode regular expression
                            static const string WC_TLS_MODE_REG_EXP_STR;

                        public:

                            //Stores the server port parameter name
                            static const string WC_SERVER_URI_PARAM_NAME;

                            //Stores the server TLS support parameter name
                            static const string WC_IS_TLS_CLIENT_PARAM_NAME;

                            //Stores the server TLS mode parameter name
                            static const string WC_TLS_MODE_PARAM_NAME;

                            //Stores the name of the server
                            string m_server_name;
                            //The port to listen to
                            string m_server_uri;
                            //The flag indicating whether the TLS server is running
                            bool m_is_tls_client;
                            //Stores the TLS mode for the case of the TLS server
                            tls_mode_enum m_tls_mode;
                            //Stores the TLS mode string name for the case of the TLS server
                            string m_tls_mode_name;
                            //Stores the client's ciphers, or an empty string for defaults
                            string m_ciphers;

                            /**
                             * The basic constructor, finalization is required!
                             */
                            websocket_client_params_struct() :
                            m_server_name(""),
                            m_server_uri(""), m_is_tls_client(false),
                            m_tls_mode(tls_mode_enum::MOZILLA_UNDEFINED),
                            m_tls_mode_name(tls_val_to_str(tls_mode_enum::MOZILLA_UNDEFINED)) {
                            }

                            /**
                             * The basic constructor, finalization is required!
                             * @param server_name the server name used for logging
                             */
                            websocket_client_params_struct(const string server_name) :
                            m_server_name(server_name),
                            m_server_uri(""), m_is_tls_client(false),
                            m_tls_mode(tls_mode_enum::MOZILLA_UNDEFINED),
                            m_tls_mode_name(tls_val_to_str(tls_mode_enum::MOZILLA_UNDEFINED)) {
                            }

                            /**
                             * The basic constructor, finalization is required!
                             * @param server_name the server name used for logging
                             * @param server_uri the server URI
                             * @param tls_mode_name the server TLS mode
                             */
                            websocket_client_params_struct(
                                    const string & server_name,
                                    const string & server_uri,
                                    const string & tls_mode_name) :
                            m_server_name(server_name),
                            m_server_uri(server_uri), m_is_tls_client(false),
                            m_tls_mode(tls_mode_enum::MOZILLA_UNDEFINED),
                            m_tls_mode_name(tls_mode_name) {
                            }

                            /**
                             * Allows to finalize the parameters after loading.
                             */
                            virtual void finalize() {
                                //Check if the server URI is correct
                                const regex server_uri_regexp(WC_SERVER_URI_REG_EXP_STR);
                                ASSERT_CONDITION_THROW(!regex_match(m_server_uri, server_uri_regexp),
                                        string("The server URI: '") + m_server_uri +
                                        string("' of server '") + m_server_name +
                                        string("' does not match the allowed pattern: ") +
                                        WC_SERVER_URI_REG_EXP_STR);

                                //Detect whether we need to use TLS client or not
                                const regex is_tls_client_regexp(WC_IS_TLS_CLIENT_REG_EXP_STR);
                                m_is_tls_client = regex_match(m_server_uri, is_tls_client_regexp);

                                //Check if the client was built with TLS support
#if (!defined(WITH_TLS) || !WITH_TLS)
                                //If his is a TLS disabled client
                                ASSERT_CONDITION_THROW(m_is_tls_client,
                                        string("According to the server URI: '") + m_server_uri +
                                        string("' the TLS client is requested but the client ") +
                                        string("is not built with TLS support!"));
#else
                                //If this is a TLS enabled client
                                if (m_is_tls_client) {
                                    //If the TLS client is requested check on the TLS mode
                                    const regex tls_mode_name_regexp(WC_TLS_MODE_REG_EXP_STR);
                                    ASSERT_CONDITION_THROW(
                                            (!regex_match(m_tls_mode_name, tls_mode_name_regexp)),
                                            string("The server TLS mode name: '") +
                                            m_tls_mode_name + string("' does not match ") +
                                            string("the allowed pattern: '") +
                                            WC_TLS_MODE_REG_EXP_STR + string("'!"));
                                    //Convert the TLS mode name into its value
                                    m_tls_mode = tls_str_to_val(m_tls_mode_name);

                                    //Check if the mode is set to undefined, this is not allowed
                                    ASSERT_CONDITION_THROW((m_tls_mode == tls_mode_enum::MOZILLA_UNDEFINED),
                                            string("According to the server URI: '") +
                                            m_server_uri + string("' the TLS client ") +
                                            string("is requested but the TLS mode ") +
                                            string("is set to: '") + m_tls_mode_name + string("'"));
                                } else {
                                    //If a non-TLS client is requested then get the TLS mode
                                    m_tls_mode = tls_str_to_val(m_tls_mode_name);
                                    if (m_tls_mode != tls_mode_enum::MOZILLA_UNDEFINED) {
                                        //If the mode is not undefined then report
                                        //a warning and set it to undefined

                                        m_tls_mode = tls_mode_enum::MOZILLA_UNDEFINED;
                                        m_tls_mode_name = tls_val_to_str(m_tls_mode);
                                        LOG_WARNING << "According to the server URI: '"
                                                << m_server_uri << "' the non-TLS client "
                                                << "is requested but the TLS mode "
                                                << "is set to: " << m_tls_mode_name
                                                << ", resetting it to: "
                                                << tls_val_to_str(m_tls_mode) << END_LOG;
                                    }
                                }
#endif
                            }

                            /**
                             * The assignment operator
                             * @param other the object to assign from
                             * @return the reference to this object
                             */
                            websocket_client_params_struct & operator=(const websocket_client_params_struct & other) {
                                this->m_server_name = other.m_server_name;
                                this->m_server_uri = other.m_server_uri;
                                this->m_is_tls_client = other.m_is_tls_client;
                                this->m_tls_mode = other.m_tls_mode;
                                this->m_tls_mode_name = other.m_tls_mode_name;
                                return *this;
                            }
                        };

                        //Typedef the structure
                        typedef websocket_client_params_struct websocket_client_params;

                        /**
                         * Allows to output the parameters object to the stream
                         * @param stream the stream to output into
                         * @param params the parameters object
                         * @return the stream that we output into
                         */
                        static inline std::ostream& operator<<(
                                std::ostream& stream, const websocket_client_params & params) {
                            //Dump the parameters
                            stream << "WebSocket client parameters: {"
                                    << websocket_client_params::WC_SERVER_URI_PARAM_NAME
                                    << " = " << params.m_server_uri
                                    << ", " << websocket_client_params::WC_IS_TLS_CLIENT_PARAM_NAME
                                    << " = ";
                            if (params.m_is_tls_client) {
                                stream << "true"
                                        << ", " << websocket_client_params::WC_TLS_MODE_PARAM_NAME
                                        << " = " << tls_val_to_str(params.m_tls_mode);
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

#endif /* WEBSOCKET_CLIENT_PARAMS_HPP */

