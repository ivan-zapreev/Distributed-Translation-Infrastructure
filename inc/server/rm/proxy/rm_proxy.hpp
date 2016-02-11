/* 
 * File:   rm_proxy.hpp
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

#ifndef RM_PROXY_HPP
#define RM_PROXY_HPP

#include "server/rm/proxy/rm_query_proxy.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace proxy {
                        
                        /**
                         * This is the reordering model proxy interface class it allows to
                         * interact with any sort of local and remote models in a uniform way.
                         * @param num_weights is the number of reordering weights
                         */
                        template<size_t num_weights>
                        class rm_proxy {
                        public:
                            //Stores the number of weights for external use
                            static constexpr size_t NUM_WEIGHTS = num_weights;
                            
                            //Declare the query proxy type
                            typedef rm_query_proxy<num_weights> rm_num_query_proxy;
                            
                            /**
                             * Allows to connect to the model object based on the given parameters
                             * @param conn_str the connection string
                             */
                            virtual void connect(const string & conn_str) = 0;

                            /**
                             * Allows to disconnect from the trie
                             */
                            virtual void disconnect() = 0;
                            
                            /**
                             * The basic virtual destructor
                             */
                            virtual ~rm_proxy(){};

                            /**
                             * This method allows to get a query executor for the given trie
                             * @return the trie query proxy object
                             */
                            virtual rm_num_query_proxy * get_query_proxy() = 0;
                        };

                        template<size_t num_weights>
                        constexpr size_t rm_proxy<num_weights>::NUM_WEIGHTS;
                    }
                }
            }
        }
    }
}


#endif /* RM_PROXY_HPP */

