/* 
 * File:   balancer_parameters.hpp
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
 * Created on July 8, 2016, 10:36 AM
 */

#ifndef BALANCER_PARAMETERS_HPP
#define BALANCER_PARAMETERS_HPP

#include <map>
#include <string>
#include <regex>

#include "common/messaging/websocket/websocket_client_params.hpp"
#include "common/messaging/websocket/websocket_server_params.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::common::messaging::websocket;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This structure stores the configuration/connection
                 * parameters of the translation servers.
                 */
                struct translator_config_struct : public websocket_client_params {
                    //Stores the load factor parameter name
                    static const string TC_LOAD_WEIGHT_PARAM_NAME;

                    //Stores the load weight factor for the server,
                    //should be a value >= 0. If set to 0 then the
                    //translation server will not be used at all.
                    uint32_t m_load_weight;

                    /**
                     * The default constructor, finalization is required!
                     */
                    translator_config_struct() :
                    websocket_client_params(), m_load_weight(0) {
                    }

                    /**
                     * The default constructor setting the server
                     * name, finalization is required!
                     * @param server_name the server name used for logging
                     */
                    translator_config_struct(const string & server_name) :
                    websocket_client_params(server_name), m_load_weight(0) {
                    }

                    /**
                     * The default constructor setting the server
                     * name, finalization is required!
                     * @param server_name the server name used for logging
                     * @param server_uri the server URI
                     * @param tls_mode_name the server TLS mode
                     * @param tls_ciphers the TLS ciphers, or empty string for using system defaults
                     * @param load_weight the server load weight
                     */
                    translator_config_struct(
                            const string & server_name,
                            const string & server_uri,
                            const string & tls_mode_name,
                            const string & tls_ciphers,
                            const uint32_t load_weight) :
                    websocket_client_params(
                    server_name, server_uri, tls_mode_name, tls_ciphers),
                    m_load_weight(load_weight) {
                    }

                    /**
                     * @see websocket_client_params
                     */
                    virtual void finalize() override {
                        websocket_client_params::finalize();

                        //Check on the load weight
                        ASSERT_CONDITION_THROW((m_load_weight < 0),
                                string("The server load weight in '") + m_server_name +
                                string("' is negative (") + to_string(m_load_weight)+(")! "));
                    }

                    /**
                     * The assignment operator
                     * @param other the object to assign from
                     * @return the reference to this object
                     */
                    translator_config_struct & operator=(const translator_config_struct & other) {
                        ((websocket_client_params) * this) = (websocket_client_params) other;
                        this->m_load_weight = other.m_load_weight;
                        return *this;
                    }
                };

                //Typedef the structure
                typedef translator_config_struct trans_server_params;

                /**
                 * This is the storage for balancer parameters:
                 * Responsibilities:
                 *      Store the run-time parameters of the balancer application
                 */
                struct balancer_parameters_struct : public websocket_server_params {
                    //Stores the configuration section name
                    static const string SE_CONFIG_SECTION_NAME;
                    //Stores the server port parameter name
                    static const string SE_SERVER_PORT_PARAM_NAME;
                    //Stores the server TLS support parameter name
                    static const string SE_IS_TLS_SERVER_PARAM_NAME;
                    //Stores the number of request threads parameter name
                    static const string SE_NUM_REQ_THREADS_PARAM_NAME;
                    //Stores the number of response threads parameter name
                    static const string SE_NUM_RESP_THREADS_PARAM_NAME;
                    //Stores the translation server names parameter name
                    static const string SE_TRANSLATION_SERVER_NAMES_PARAM_NAME;
                    //Stores the server reconnection time out parameter name
                    static const string SC_RECONNECT_TIME_OUT_PARAM_NAME;

                    //The delimiter for the translation server names
                    static const string TRANS_SERV_NAMES_DELIMITER_STR;

                    //The number of the threads handling the request queue
                    size_t m_num_req_threads;

                    //The number of the threads handling the response queue
                    size_t m_num_resp_threads;

                    //The number of milliseconds to wait before we attempt to
                    //reconnect to a disconnected translation server.
                    uint32_t m_recon_time_out;

                    //Stores the mapping from the translation server name to its configuration data
                    map<string, trans_server_params> m_trans_servers;

                    /**
                     * The basic constructor
                     */
                    balancer_parameters_struct() {
                    }

                    /**
                     * Allows to add a new translator configuration.
                     * @param params the server's parameters
                     */
                    inline void add_translator(
                            trans_server_params & params) {
                        //Store the data object
                        m_trans_servers.insert(
                                pair<string, trans_server_params>(
                                params.m_server_name, params));
                    }

                    /**
                     * Allows to change the weight of the given translation server
                     * @param name the name of the server to change
                     * @param load_weight the new load weight
                     */
                    inline void change_weight(const string & name, const float load_weight) {
                        //Check on the load weight
                        ASSERT_CONDITION_THROW((load_weight < 0),
                                string("The server load weight in '") + name +
                                string("' is negative (") + to_string(load_weight)+(")! "));

                        //Check that the server with this name is there
                        auto iter = m_trans_servers.find(name);
                        ASSERT_CONDITION_THROW((iter == m_trans_servers.end()),
                                string("The server: '") + name + string("' is not found!"));

                        //Set the new load weight and total weight
                        iter->second.m_load_weight = load_weight;
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    virtual void finalize() override {
                        websocket_server_params::finalize();

                        ASSERT_CONDITION_THROW((m_num_req_threads == 0),
                                string("The number of request threads: ") +
                                to_string(m_num_req_threads) +
                                string(" must be larger than zero! "));

                        ASSERT_CONDITION_THROW((m_num_resp_threads == 0),
                                string("The number of response threads: ") +
                                to_string(m_num_resp_threads) +
                                string(" must be larger than zero! "));

                        ASSERT_CONDITION_THROW((m_recon_time_out <= 0),
                                string("Invalid reconnection time out: ") +
                                to_string(m_recon_time_out) + string(" must be > 0!"));

                        //Iterate over the clients and finalize them as well
                        for (auto iter = m_trans_servers.begin(); iter != m_trans_servers.end(); ++iter) {
                            iter->second.finalize();
                        }
                    }
                };

                //Typedef the structure
                typedef balancer_parameters_struct balancer_parameters;

                /**
                 * Allows to output the parameters object to the stream
                 * @param stream the stream to output into
                 * @param params the parameters object
                 * @return the stream that we output into
                 */
                static inline std::ostream& operator<<(std::ostream& stream, const trans_server_params & params) {
                    return stream << "{ " << params.m_server_name << ", "
                            << params.m_server_uri << ", load weight="
                            << params.m_load_weight << " }";
                }

                /**
                 * Allows to output the parameters object to the stream
                 * @param stream the stream to output into
                 * @param params the parameters object
                 * @return the stream that we output into
                 */
                static inline std::ostream& operator<<(std::ostream& stream, const balancer_parameters & params) {
                    //Dump the main server configuration
                    stream << "Balancer parameters: { "
                            << (websocket_server_params) params
                            << ", " << balancer_parameters::SE_NUM_REQ_THREADS_PARAM_NAME
                            << " = " << params.m_num_req_threads
                            << ", " << balancer_parameters::SE_NUM_RESP_THREADS_PARAM_NAME
                            << " = " << params.m_num_resp_threads
                            << ", translation servers (" << params.m_trans_servers.size(
                            ) << ") = [ ";
                    //Dump the translation server's configurations
                    for (auto iter = params.m_trans_servers.begin(); iter != params.m_trans_servers.end(); ++iter) {
                        stream << iter->second << ", ";
                    }
                    //Finish the dump
                    return stream << " ] }";
                }
            }
        }
    }
}

#endif /* BALANCER_PARAMETERS_HPP */

