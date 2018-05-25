/* 
 * File:   websocket_parameters.hh
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

#ifndef WEBSOCKET_PARAMETERS_HPP
#define WEBSOCKET_PARAMETERS_HPP

#include <string>
#include <fstream>
#include <regex>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the storage for websocket server parameters:
                     * Responsibilities:
                     *      Store the run-time parameters of the websocket server
                     */
                    struct websocket_parameters_struct {
                        //Stores the server port parameter name
                        static const string SE_SERVER_PORT_PARAM_NAME;
                        //Stores the server TLS support parameter name
                        static const string SE_IS_TLS_SERVER_PARAM_NAME;
                        //Stores the server TLS certificate parameter name
                        static const string SE_TLS_CERT_FILE_PARAM_NAME;
                        //Stores the server TLS private key parameter name
                        static const string SE_TLS_KEY_FILE_PARAM_NAME;
                        //Stores the TLS temporary DH pem parameter name
                        static const string SE_TLS_DH_FILE_PARAM_NAME;
                        //The server TLS certificate file name regular expression
                        static const string SE_TLS_CERT_FILE_REG_EXP_STR;
                        //The server TLS private key file name regular expression
                        static const string SE_TLS_KEY_FILE_REG_EXP_STR;
                        //The TLS temporary DH pem file name regular expression
                        static const string SE_TLS_DH_FILE_REG_EXP_STR;

                        //The port to listen to
                        uint16_t m_server_port;
                        //The flag indicating whether the TLS server is running
                        bool m_is_tls_server;
                        //Stores the server's certificate file name
                        string m_server_crt_file;
                        //Stores the server's private key file name
                        string m_priv_key_file;
                        //Stores the server's DH pem file name
                        string m_tmp_dh_pem_file;

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
#if !defined(WITH_TLS) || !WITH_TLS
                            if (m_is_tls_server) {
                                LOG_WARNING << "The value of the "
                                        << SE_IS_TLS_SERVER_PARAM_NAME
                                        << " is set to TRUE but the server is not"
                                        << " compiled with TLS support, re-setting"
                                        << " to FALSE!" << END_LOG;
                                m_is_tls_server = false;
                            }
#else
                            if (m_is_tls_server) {
                                const regex server_crt_regexp(SE_TLS_CERT_FILE_REG_EXP_STR);
                                ASSERT_CONDITION_THROW(
                                        (!regex_match(m_server_crt_file, server_crt_regexp)),
                                        string("The server TLS certificate file name: ") +
                                        m_server_crt_file + string(" does not match ") +
                                        string(" the allowed extensions: '") +
                                        SE_TLS_CERT_FILE_REG_EXP_STR + string("'!"));
                                ASSERT_CONDITION_THROW((!file_exists(m_server_crt_file)),
                                        string("The server TLS certificate file: ") +
                                        m_server_crt_file + string(" does not exist! "));

                                const regex priv_key_regexp(SE_TLS_KEY_FILE_REG_EXP_STR);
                                ASSERT_CONDITION_THROW(
                                        (!regex_match(m_priv_key_file, priv_key_regexp)),
                                        string("The server TLS private key file: ") +
                                        m_priv_key_file + string(" does not match ") +
                                        string(" the allowed extensions: '") +
                                        SE_TLS_KEY_FILE_REG_EXP_STR + string("'!"));
                                ASSERT_CONDITION_THROW((!file_exists(m_priv_key_file)),
                                        string("The server TLS private key file: ") +
                                        m_priv_key_file + string(" does not exist! "));

                                const regex tmp_dh_pem_regexp(SE_TLS_DH_FILE_REG_EXP_STR);
                                ASSERT_CONDITION_THROW(
                                        (!regex_match(m_tmp_dh_pem_file, tmp_dh_pem_regexp)),
                                        string("The server TLS tmp DH pem file: ") +
                                        m_tmp_dh_pem_file + string(" does not match ") +
                                        string(" the allowed extensions: '") +
                                        SE_TLS_DH_FILE_REG_EXP_STR + string("'!"));
                                ASSERT_CONDITION_THROW((!file_exists(m_tmp_dh_pem_file)),
                                        string("The server TLS tmp DH pem file: ") +
                                        m_tmp_dh_pem_file + string(" does not exist! "));
                            }
#endif
                        }
                    };

                    //Typedef the structure
                    typedef websocket_parameters_struct websocket_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const websocket_parameters & params) {
                        //Dump the parameters
                        stream << "Processor parameters: {"
                                << websocket_parameters::SE_SERVER_PORT_PARAM_NAME
                                << " = " << params.m_server_port
                                << ", " << websocket_parameters::SE_IS_TLS_SERVER_PARAM_NAME
                                << " = ";
                        if (params.m_is_tls_server) {
                            stream << "true"
                                    << ", " << websocket_parameters::SE_TLS_CERT_FILE_PARAM_NAME
                                    << " = " << params.m_server_crt_file
                                    << ", " << websocket_parameters::SE_TLS_KEY_FILE_PARAM_NAME
                                    << " = " << params.m_priv_key_file
                                    << ", " << websocket_parameters::SE_TLS_DH_FILE_PARAM_NAME
                                    << " = " << params.m_tmp_dh_pem_file;
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

#endif /* WEBSOCKET_PARAMETERS_HPP */

