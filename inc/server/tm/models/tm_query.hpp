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

#include<string>
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
        namespace bpbd {
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
                        template<typename model_type>
                        class tm_query {
                        public:
                            //Define the query map as a mapping from the source phrase
                            //id to the pointer to the constant translation data. 
                            typedef unordered_map<phrase_uid, const tm_source_entry *> query_map;

                            /**
                             * The basic constructor
                             */
                            tm_query(const model_type & model) : m_model(model) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_query() {
                            }

                            /**
                             * Allows to execute the query
                             */
                            inline void execute(){
                                //ToDo: Implement querying the translation model
                            }
                            
                            /**
                             * Allows to add the source phrase to the query.
                             * @param uid the source phrase uid
                             */
                            inline void add_source(const phrase_uid uid) {
                                //Check if there is already a phrase with this id present
                                query_map::iterator iter = m_query_data.find(uid);
                                if (iter == m_query_data.end()) {
                                    //If the id is not present then set the value to NULL in order to add it.
                                    //ToDo: Set it to point to the UNK source entry right away.
                                    iter->second = NULL;
                                }
                            }

                            /**
                             * Allows to add the source phrase to the query.
                             * Note that the source phrase it taken as is,
                             * i,e. no additional trimming is done.
                             * @param source the source phrase to be added to the query
                             */
                            inline void add_source(const string & source) {
                                add_source(get_phrase_uid(source));
                            }

                            /**
                             * Allows to get the target translations for the source phrase
                             * @param uid the source phrase uid
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the translation was not found.
                             */
                            inline const tm_source_entry * get_targets(const phrase_uid uid) {
                                //Check that the source phrase is present, we are not allowed
                                //to translate something that was not added to the query!
                                ASSERT_SANITY_THROW((m_query_data.find(uid) == m_query_data.end()),
                                        string("The source with uid: ") + to_string(uid) +
                                        string(" is not part of the translation query!"));
                                return m_query_data[uid];
                            }

                            /**
                             * Allows to get the target translations for the source phrase
                             * @param source the source phrase to get translations for
                             * @return the pointer to the source entry map or NULL if no translation is found
                             */
                            inline const tm_source_entry * get_targets(const string & source) {
                                return get_targets(get_phrase_uid(source));
                            }

                        private:
                            //Stores the reference to the translation model
                            const model_type & m_model;
                            //Stores the mapping from the source phrase id to the corresponding source entry.
                            query_map m_query_data;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_QUERY_HPP */

