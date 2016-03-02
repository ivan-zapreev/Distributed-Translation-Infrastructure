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
#include<vector>

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
                            typedef unordered_map<phrase_uid, tm_const_source_entry_ptr> query_map;

                            /**
                             * The basic constructor
                             */
                            tm_query(const model_type & model) : m_model(model) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_query() {
                                //Nothing to be done
                            }

                            /**
                             * Allows to execute the translation query for the given source phrase.
                             * This query also keeps the local cache of retrieved source phrase translations.
                             * @param uid [in] the source phrase uid
                             * @param entry_ptr [out] the reference to the source entry pointer
                             * which will be initialized with the found source entry.
                             */
                            inline void execute(const phrase_uid uid, tm_const_source_entry_ptr & entry_ptr) {
                                LOG_DEBUG1 << "Requesting the translation for the phrase uid: " << uid << END_LOG;

                                //Check if there has been already retrieved data for this uid
                                query_map::iterator iter = m_query_data.find(uid);

                                //If there has not been retrieved anything for this phrase then ask the model                                
                                if (iter == m_query_data.end()) {
                                    //Search the model and store the pointer to the found entry
                                    entry_ptr = m_model.template get_source_entry<true>(uid);

                                    //Store the pointer into the local map
                                    m_query_data[uid] = entry_ptr;

                                    LOG_DEBUG1 << "The translation source entry for: " << uid << " is retrieved!" << END_LOG;

                                    //Perform the sanity check for the sake of safety
                                    ASSERT_SANITY_THROW((entry_ptr == NULL),
                                            string("Got a NULL pointer for the ") + to_string(uid) +
                                            string(" translations, broken translation model implementation!"));
                                } else {
                                    LOG_DEBUG1 << "Setting the source entry pointer to the [out] pointer!" << END_LOG;
                                    //Set the pointer to the proper entry
                                    entry_ptr = iter->second;
                                }
                            }

                            /**
                             * Allows retrieve the UNK phrase entry
                             * @return the pointer to the UNK entry
                             */
                            tm_const_source_entry * get_unk_entry() {
                                return m_model.get_unk_entry();
                            }

                            /**
                             * Allows to get translations for the given source entry
                             * @param uid the unique identifier of the source phrase
                             * @return the pointer to the source entry or NULL if the translation is not found
                             */
                            inline tm_const_source_entry * get_source_entry(const phrase_uid uid) {
                                LOG_DEBUG1 << "Getting translations for the phrase uid: " << uid << END_LOG;
                                
                                return m_model.template get_source_entry<false>(uid);
                            }

                            /**
                             * Allows to get all the source/target phrase identifiers
                             * for the source target translation in this query.
                             * Must be called after the query is executed
                             * @param st_uids the container for the source/target phrase identifiers
                             */
                            inline void get_st_uids(vector<phrase_uid> & st_uids) const {
                                for (query_map::const_iterator iter = m_query_data.begin(); iter != m_query_data.end(); ++iter) {
                                    iter->second->get_st_uids(st_uids);
                                }
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

