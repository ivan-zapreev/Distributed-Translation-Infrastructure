/* 
 * File:   translation_servers_manager.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef TRANSLATION_SERVERS_MANAGER_HPP
#define TRANSLATION_SERVERS_MANAGER_HPP

#include <map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "balancer/balancer_parameters.hpp"
#include "balancer/translation_server_adapter.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the translation servers manager class:
                 * Responsibilities:
                 *      Keeps track of the online translation servers
                 *      Keeps track of languages supported by the servers
                 *      Keeps track of the known load on the servers
                 *      Advises translation server for a translation request
                 */
                class translation_servers_manager {
                public:
                    
                    /**
                     * Allows to configure the balancer server
                     * @param params the parameters from which the server will be configured
                     */
                    static inline void configure(const balancer_parameters & params) {
                        //Iterate through the list of translation server
                        //configs and create an adapter for each of them
                        for(auto iter = params.trans_servers.begin(); iter != params.trans_servers.end(); ++ iter) {
                            m_server_adaptors[iter->first].configure(iter->second);
                        }
                    }

                    /**
                     * The main method to start the translation servers manager
                     */
                    static inline void start() {
                        for(auto iter = m_server_adaptors.begin(); iter != m_server_adaptors.end(); ++ iter) {
                            iter->second.start();
                        }
                    }

                    /**
                     * Allows to stop the translation servers manager
                     */
                    static inline void stop() {
                        for(auto iter = m_server_adaptors.begin(); iter != m_server_adaptors.end(); ++ iter) {
                            iter->second.stop();
                        }
                    }

                private:
                    //Stores the mapping from the server names to the server adaptors
                    static map<string, translation_server_adapter> m_server_adaptors;
                    
                    //Stores the mapping from the source/target language pairs to the adaptor sets

                    /**
                     * The private constructor to keep the class from being instantiated
                     */
                    translation_servers_manager() {
                    }
                };
            }
        }
    }
}
#endif /* TRANSLATION_SERVERS_MANAGER_HPP */

