/* 
 * File:   tm_proxy_local.hpp
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
 * Created on February 8, 2016, 9:59 AM
 */

#ifndef TM_PROXY_LOCAL_HPP
#define TM_PROXY_LOCAL_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/monitore/statistics_monitore.hpp"
#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/tm/proxy/tm_query_proxy.hpp"
#include "server/tm/proxy/tm_query_proxy_local.hpp"

#include "server/tm/builders/tm_basic_builder.hpp"

using namespace uva::utils::monitore;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::translation::server::tm;
using namespace uva::smt::translation::server::tm::proxy;
using namespace uva::smt::translation::server::tm::builders;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {
                    namespace proxy {

                        /**
                         * This is the translation model proxy interface class it allows to
                         * interact with any sort of local and remote models in a uniform way.
                         */
                        template<typename model_type>
                        class tm_proxy_local : public tm_proxy {
                        public:

                            /**
                             * The basic proxy constructor, currently does nothing except for default initialization
                             */
                            tm_proxy_local() {
                            }

                            /**
                             * Allows to connect to the model object based on the given parameters
                             * @param the parameters defining the model to connect to
                             */
                            virtual void connect(const tm_parameters & params){
                                //ToDo: load the translation model into the memory
                            }

                            /**
                             * Allows to disconnect from the trie
                             */
                            virtual void disconnect() {
                                //Nothing the be done here yet, the model is allocated on the stack
                            }

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~tm_proxy_local() {
                                //Disconnect, just in case it has not been done before
                                disconnect();
                            };

                            /**
                             * This method allows to get a query executor for the given trie
                             * @return the trie query proxy object
                             */
                            virtual tm_query_proxy * get_query_proxy() {
                                //ToDo: Return an instance of the query_proxy_local class
                                return NULL;
                            }

                            /**
                             * Allows to log the trie type usage information
                             */
                            virtual void log_model_type_info(){
                                //ToDo: Implement logging the translation model information
                            }
                            
                            private:
                                //Stores the translation model instance
                                model_type m_model;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_PROXY_LOCAL_HPP */

