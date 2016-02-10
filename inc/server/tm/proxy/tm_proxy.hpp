/* 
 * File:   tm_proxy.hpp
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
 * Created on February 8, 2016, 9:58 AM
 */

#ifndef TM_PROXY_HPP
#define TM_PROXY_HPP

#include "server/tm/tm_parameters.hpp"
#include "server/tm/proxy/tm_query_proxy.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace proxy {
                        
                        /**
                         * This is the translation model proxy interface class it allows to
                         * interact with any sort of local and remote models in a uniform way.
                         */
                        class tm_proxy {
                        public:
                            
                            /**
                             * Allows to connect to the model object based on the given parameters
                             * @param the parameters defining the model to connect to
                             */
                            virtual void connect(const tm_parameters & params) = 0;

                            /**
                             * Allows to disconnect from the trie
                             */
                            virtual void disconnect() = 0;
                            
                            /**
                             * The basic virtual destructor
                             */
                            virtual ~tm_proxy(){};

                            /**
                             * This method allows to get a query executor for the given trie
                             * @return the trie query proxy object
                             */
                            virtual tm_query_proxy * get_query_proxy() = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_PROXY_HPP */

