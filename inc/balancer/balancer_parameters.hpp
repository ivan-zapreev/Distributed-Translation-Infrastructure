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

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This structure stores the configuration/connection
                 * parameters of the translation servers.
                 */
                typedef struct {
                    
                } translator_config;
                
                /**
                 * This is the storage for balancer parameters:
                 * Responsibilities:
                 *      Store the run-time parameters of the balancer application
                 */
                struct balancer_parameters_struct {
                    //Stores the configuration section name
                    static const string SE_CONFIG_SECTION_NAME;
                    //Stores the server port parameter name
                    static const string SE_SERVER_PORT_PARAM_NAME;
                    //Stores the number of threads parameter name
                    static const string SE_NUM_THREADS_PARAM_NAME;

                    //The port to listen to
                    uint16_t m_server_port;

                    //The number of the translation threads to run
                    size_t m_num_threads;

                    //Stores the mapping from the translation server name to its configuration data
                    map<string, translator_config> translators;
                    
                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    void finalize() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
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
                static inline std::ostream& operator<<(std::ostream& stream, const balancer_parameters & params) {
                    return stream << "Server parameters: [ server_port = " << params.m_server_port
                            << ", num_threads = " << params.m_num_threads << " ]";
                }
            }
        }
    }
}

#endif /* BALANCER_PARAMETERS_HPP */

