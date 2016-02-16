/* 
 * File:   configurator.hpp
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
 * Created on February 4, 2016, 1:32 PM
 */

#ifndef LM_CONFIGURATOR_HPP
#define LM_CONFIGURATOR_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/lm_parameters.hpp"

#include "server/lm/proxy/lm_proxy.hpp"
#include "server/lm/proxy/lm_proxy_local.hpp"
#include "server/lm/proxy/lm_trie_query_proxy.hpp"

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::smt::bpbd::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This class represents a singleton that allows to
                     * configure the language model and then issues.
                     * query proxy objects for performing the queries
                     * against the internally encapsulated language model(s).
                     */
                    class lm_configurator {
                    public:

                        /**
                         * This method allows to set the configuration parameters
                         * for the word index trie etc. This method is to be called
                         * only once! The latter is not checked but is a must.
                         * @param params the language model parameters to be set.
                         */
                        static void connect(const lm_parameters & params) {
                            //Store the parameters for future use
                            m_params = params;

                            //At the moment we only support a local proxy
                            m_model_proxy = new lm_proxy_local();

                            //Connect to the trie instance using the given parameters
                            m_model_proxy->connect(m_params.m_conn_string);
                        }

                        /**
                         * Allows to disconnect from the language model.
                         */
                        static void disconnect() {
                            if (m_model_proxy != NULL) {
                                //Disconnect from the trie
                                m_model_proxy->disconnect();
                                //Delete the object, free the resources
                                delete m_model_proxy;
                                m_model_proxy = NULL;
                            }
                        }

                        /**
                         * Allows to return an instance of the query executor,
                         * is to be returned by calling the dispose method.
                         * @return an instance of the query executor.
                         */
                        static inline lm_trie_query_proxy & allocate_query_proxy() {
                            //Return the query executor as given by the proxy class
                            return m_model_proxy->allocate_trie_query_proxy();
                        }

                        /**
                         * Dispose the previously allocated query object
                         * @param query the query to dispose
                         */
                        static inline void dispose_query_proxy(lm_trie_query_proxy & query) {
                            m_model_proxy->dispose_trie_query_proxy(query);
                        }

                    private:
                        //Stores the copy of the configuration parameters
                        static lm_parameters m_params;

                        //Store the trie proxy object
                        static lm_proxy * m_model_proxy;
                    };
                }
            }
        }
    }
}

#endif /* CONFIGURATOR_HPP */

