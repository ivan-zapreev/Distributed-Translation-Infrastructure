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

#include <vector>

#include "server/common/models/phrase_id.hpp"

#include "server/rm/models/rm_entry.hpp"

using namespace std;

using namespace uva::smt::bpbd::server::common::models;

using namespace uva::smt::bpbd::server::rm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This class represents a reordering query proxy interface class.
                         * It allows to interact with reordering model queries in a uniform way.
                         * @param num_weights is the number of reordering weights
                         */
                        template<size_t num_weights>
                        class rm_query_proxy {
                        public: 
                              
                            //Typedef the entry with the template parameter
                            typedef rm_entry<num_weights> rm_num_entry;

                            /**
                             * Allows to set the source/target phrase identifiers
                             * for which the reordering data is to be retrieved.
                             * @param uids the source/target phrase identifiers
                             */
                            virtual void set_st_uids(const vector<phrase_uid> * const uids) = 0;

                            /**
                             * Allows to get the source/target reordering data from the reordering model
                             * @param uid the source/target phrase uid
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the reordering was not found.
                             */
                            virtual const rm_num_entry & get_reordering(const phrase_uid uid) = 0;
                            
                            /**
                             * Allows to execute the query 
                             */
                            virtual void execute() = 0;

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

