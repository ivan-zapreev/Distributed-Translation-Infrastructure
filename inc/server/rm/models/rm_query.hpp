/* 
 * File:   rm_query.hpp
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
 * Created on February 8, 2016, 10:03 AM
 */

#ifndef RM_QUERY_HPP
#define RM_QUERY_HPP


#include<string>
#include<unordered_map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/rm/models/rm_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {

                        /**
                         * This class represents a query for the reordering model
                         */
                        template<typename model_type>
                        class rm_query {
                        public:

                            //Define the query map as a mapping from the source/target phrase
                            //uid to the pointer to the constant reordering data. 
                            typedef unordered_map<phrase_uid, const rm_entry *> query_map;

                            /**
                             * The basic constructor
                             */
                            rm_query(const model_type & model) : m_model(model) {
                            }

                            /**
                             * Allows to execute the query, for the given source/target phrase ids
                             * @param st_ids is the list of the source/target phrase ids
                             *               for which the reordering data is needed
                             */
                            virtual void execute(const vector<phrase_uid> & st_ids) {
                                //Iterate through the source phrases and query them
                                for (vector<phrase_uid>::const_iterator iter = st_ids.begin(); iter != st_ids.end(); ++iter) {
                                    //Get the source/target phrase id
                                    const phrase_uid st_uid = *iter;
                                    //Retrieve the reordering entry from the model
                                    const rm_entry * entry = m_model.get_entry(st_uid);

                                    //Perform the sanity check for the sake of safety
                                    ASSERT_SANITY_THROW((entry == NULL),
                                            string("Got a NULL pointer for the ") + to_string(st_uid) +
                                            string(" reordering, broken reordering model implementation!"));

                                    //Put the reordering into the result map
                                    m_query_data[st_uid] = entry;
                                }
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_query() {
                                //Nothing to be done
                            }

                            /**
                             * Allows to get the source/target reordering data from the reordering model
                             * @param uid the source/target phrase uid
                             * @return the reference to the source entry, might be the one
                             *         of UNK if the reordering was not found.
                             */
                            virtual const rm_entry & get_reordering(const phrase_uid uid) const {
                                //Check that the source phrase is present, we are not allowed
                                //to translate something that was not added to the query!
                                ASSERT_SANITY_THROW((m_query_data.find(uid) == m_query_data.end()),
                                        string("The source/target with uid: ") + to_string(uid) +
                                        string(" is not part of the reordering query!"));

                                //Return the reference the to the entry, there is no need to check
                                //for the null pointer as this is done when the query is executed.
                                return *m_query_data.at(uid);
                            }

                        private:
                            //Stores the reordering model reference
                            const model_type & m_model;
                            //Stores the mapping from the source/target phrase id to the corresponding reordering entry.
                            query_map m_query_data;
                        };
                    }
                }
            }
        }
    }
}

#endif /* RM_QUERY_HPP */

