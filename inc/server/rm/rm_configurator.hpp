/* 
 * File:   rm_configurator.hpp
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
 * Created on February 4, 2016, 2:23 PM
 */

#ifndef RM_CONFIGURATOR_HPP
#define RM_CONFIGURATOR_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/rm/rm_parameters.hpp"
#include "server/rm/proxy/rm_proxy.hpp"
#include "server/rm/proxy/rm_proxy_local.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::bpbd::server::rm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {

                    /**
                     * This class represents a singleton that allows to
                     * configure the reordering model and then issue a
                     * proxy object for performing the queries against it.
                     */
                    class rm_configurator {
                    public:
                        
                        /**
                         * This method allows to connect to the reordering model.
                         * This method is to be called only once! The latter is
                         * not checked but is a must.
                         * @param params the reordering model parameters to be set, 
                         * this class only stores the referent to the parameters.
                         */
                        static void connect(const rm_parameters & params) {
                            //Store the parameters for future use
                            m_params = &params;

                            //At the moment we only support a local proxy
                            m_model_proxy = new rm_proxy_local();
                            
                            //Connect to the model instance using the given parameters
                            m_model_proxy->connect(*m_params);
                        }

                        /**
                         * Allows to disconnect from the reordering model.
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

                        /**
                         * Allows to return an instance of the query proxy,
                         * is to be returned by calling the dispose method.
                         * @return an instance of the query executor.
                         */
                        static inline rm_query_proxy & allocate_query_proxy() {
                            LOG_DEBUG2 << "Allocating a new RM query proxy" << END_LOG;
                            
                            //Return the query executor as given by the proxy class
                            return m_model_proxy->allocate_query_proxy();
                        }

                        /**
                         * Dispose the previously allocated query object
                         * @param query the query to dispose
                         */
                        static inline void dispose_query_proxy(rm_query_proxy & query) {
                            m_model_proxy->dispose_query_proxy(query);
                        }
                        
                    private:
                        //Stores the pointer to the configuration parameters
                        static const rm_parameters * m_params;
                        
                        //Store the trie proxy object
                        static rm_proxy * m_model_proxy;
                    };
                }
            }
        }
    }
}

#endif /* RM_CONFIGURATOR_HPP */

