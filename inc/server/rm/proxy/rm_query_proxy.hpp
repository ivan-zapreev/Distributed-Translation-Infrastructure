/* 
 * File:   rm_query_proxy.hpp
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

#ifndef RM_QUERY_PROXY_HPP
#define RM_QUERY_PROXY_HPP

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This class represents a reordering query proxy interface class.
                         * It allows to interact with reordering model queries in a uniform way.
                         */
                        class rm_query_proxy {
                        public: 
 
                            /**
                             * The basic virtual destructor
                             */
                            virtual ~rm_query_proxy() {
                            }
                         };
                    }
                }
            }
        }
    }
}

#endif /* RM_QUERY_PROXY_HPP */

