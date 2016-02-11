/* 
 * File:   tm_query_proxy.hpp
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

#ifndef TM_QUERY_PROXY_HPP
#define TM_QUERY_PROXY_HPP

#include "server/tm/models/tm_source_entry.hpp"

using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace proxy {

                        /**
                         * This class represents a translation query proxy interface class.
                         * It allows to interact with translation model queries in a uniform way.
                         */
                        class tm_query_proxy {
                        public: 
 
                            /**
                             * Allows to add the source phrase to the query.
                             * @param uid the source phrase uid
                             */
                            virtual void add_source(const phrase_uid uid) = 0;

                            /**
                             * Allows to add the source phrase to the query.
                             * Note that the source phrase it taken as is,
                             * i,e. no additional trimming is done.
                             * @param source the source phrase to be added to the query
                             */
                            virtual void add_source(const string & source) = 0;

                            /**
                             * Allows to get the target translations for the source phrase
                             * @param uid the source phrase uid
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the translation was not found.
                             */
                            virtual const tm_source_entry & get_targets(const phrase_uid uid) = 0;

                            /**
                             * Allows to get the target translations for the source phrase
                             * @param source the source phrase
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the translation was not found.
                             */
                            virtual const tm_source_entry & get_targets(const string & source) = 0;
                            
                            /**
                             * Allows to execute the query 
                             */
                            virtual void execute() = 0;
                            
                            /**
                             * The basic virtual destructor
                             */
                            virtual ~tm_query_proxy() {
                            }
                         };
                    }
                }
            }
        }
    }
}

#endif /* TM_QUERY_PROXY_HPP */

