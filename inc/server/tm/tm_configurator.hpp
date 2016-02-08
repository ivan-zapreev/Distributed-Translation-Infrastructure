/* 
 * File:   tm_configurator.hpp
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
 * Created on February 4, 2016, 2:24 PM
 */

#ifndef TM_CONFIGURATOR_HPP
#define TM_CONFIGURATOR_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/tm/tm_parameters.hpp"
#include "server/tm/proxy/tm_proxy.hpp"
#include "server/tm/proxy/tm_proxy_local.hpp"
#include "server/tm/proxy/tm_query_proxy.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::translation::server::tm::proxy;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {

                    /**
                     * This class represents a singleton that allows to
                     * configure the translation model and then issue a
                     * proxy object for performing the queries against it.
                     */
                    class tm_configurator {
                    public:
                        
                        /**
                         * This method allows to connect to the translation model.
                         * This method is to be called only once! The latter is
                         * not checked but is a must.
                         * @param params the translation model parameters to be set.
                         */
                        static void connect(const tm_parameters & params) {
                            //Store the parameters for future use
                            m_params = params;

                            //At the moment we only support a local proxy,
                            //no remotely hosted translation models
                            m_model_proxy = new tm_proxy_local();
                            
                            //Connect to the model instance using the given parameters
                            m_model_proxy->connect(m_params);
                        }

                        /**
                         * Allows to disconnect from the translation model.
                         */
                        static void disconnect() {
                            if( m_model_proxy != NULL) {
                                //Disconnect from the model
                                m_model_proxy->disconnect();
                                //Delete the object, free the resources
                                delete m_model_proxy;
                                m_model_proxy = NULL;
                            }
                        }
                        
                    protected:
                        
                    private:
                        //Stores the copy of the configuration parameters
                        static tm_parameters m_params;
                        
                        //Store the trie proxy object
                        static tm_proxy * m_model_proxy;
                    };
                }
            }
        }
    }
}

#endif /* TM_CONFIGURATOR_HPP */

