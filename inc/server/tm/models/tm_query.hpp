/* 
 * File:   tm_query.hpp
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
 * Created on February 8, 2016, 10:04 AM
 */

#ifndef TM_QUERY_HPP
#define TM_QUERY_HPP

#include<unordered_map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::containers;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {
                    namespace models {

                        /**
                         * This class represents a query for the translation model
                         * In essence it is a map from the source phrases to the
                         * pointers to maps storing the translations in the target
                         * language. The query object is not re-usable at the moment
                         * as during the translation all source translations are
                         * retrieved once at the beginning of decoding. This query
                         * is based on hashing, i.e. internally the source phrase
                         * is stored as a hash value. This might occasionally cause
                         * collisions, but since this is a local issue it can be
                         * changed in the future.
                         */
                        class tm_query {
                        public:
                            //Define the query map as a mapping from the source phrase
                            //id to the pointer to the constant translation data. 
                            typedef unordered_map<phrase_uid, const tm_source_entry *> query_map;
                            
                            /**
                             * The basic constructor
                             */
                            tm_query(){}

                            /**
                             * The basic destructor
                             */
                            ~tm_query(){}
                            
                            /**
                             * Allows to add the source phrase to the query
                             * @param source_phrase the source phrase to be added to the query
                             */
                            void add_source_phrase(string & source_phrase){
                                //ToDo: Implement
                            }
                            
                            /**
                             * Allows to get the target translations for the source phrase
                             * 
                             * ToDo: What to do if there is no translation? Return some dummy result for the UNK?
                             * 
                             * @param source_phrase the source phrase to get translations for
                             * @return the pointer to the source entry map or NULL if no translation is found
                             */
                            const tm_source_entry * get_targets(string & source_phrase) {
                                //ToDo: Implement
                                return NULL;
                            }
                            
                        protected:
                        private:
                            //
                            query_map m_query_data;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_QUERY_HPP */

