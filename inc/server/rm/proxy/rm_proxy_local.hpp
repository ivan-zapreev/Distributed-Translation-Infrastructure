/* 
 * File:   rm_proxy_local.hpp
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
 * Created on February 8, 2016, 10:00 AM
 */

#ifndef RM_PROXY_LOCAL_HPP
#define RM_PROXY_LOCAL_HPP

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/monitore/statistics_monitore.hpp"
#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/rm/proxy/rm_query_proxy.hpp"
#include "server/rm/proxy/rm_query_proxy_local.hpp"

#include "server/rm/builders/rm_basic_builder.hpp"

using namespace uva::utils::monitore;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::translation::server::rm;
using namespace uva::smt::translation::server::rm::proxy;
using namespace uva::smt::translation::server::rm::builders;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This is the reordering model proxy interface class it allows to
                         * interact with any sort of local and remote models in a uniform way.
                         */
                        template<typename model_type>
                        class rm_proxy_local : public rm_proxy {
                        public:

                            /**
                             * The basic proxy constructor, currently does nothing except for default initialization
                             */
                            rm_proxy_local() {
                            }

                            /**
                             * Allows to connect to the model object based on the given parameters
                             * @param the parameters defining the model to connect to
                             */
                            virtual void connect(const rm_parameters & params){
                                //ToDo: load the reordering model into the memory
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
                            virtual ~rm_proxy_local() {
                                //Disconnect, just in case it has not been done before
                                disconnect();
                            };

                            /**
                             * This method allows to get a query executor for the given trie
                             * @return the trie query proxy object
                             */
                            virtual rm_query_proxy * get_query_proxy() {
                                //ToDo: Return an instance of the query_proxy_local class
                                return NULL;
                            }

                            /**
                             * Allows to log the trie type usage information
                             */
                            virtual void log_model_type_info(){
                                //ToDo: Implement logging the reirdering model information
                            }
                            
                            private:
                                //Stores the reordering model instance
                                model_type m_model;
                        };
                    }
                }
            }
        }
    }
}


#endif /* RM_PROXY_LOCAL_HPP */

