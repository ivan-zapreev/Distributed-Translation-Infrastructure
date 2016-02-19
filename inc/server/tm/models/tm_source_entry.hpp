/* 
 * File:   tm_source_entry.hpp
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
 * Created on February 9, 2016, 5:34 PM
 */

#ifndef TM_SOURCE_ENTRY_HPP
#define TM_SOURCE_ENTRY_HPP

#include <string>

#include "server/tm/tm_configs.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/containers/fixed_size_hashmap.hpp"

#include "server/common/models/phrase_uid.hpp"
#include "server/tm/models/tm_target_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        /**
                         * This is the source entry data structure that contains two things
                         * The source phrase uid, which is the unique identifier of the
                         * source string and the map storing the target translations.
                         * Note that the source phrase is not stored, this is to reduce
                         * memory consumption and improve speed. Similar as we did for
                         * the g2dm tried implementation for the language model.
                         */
                        class tm_source_entry {
                        public:

                            /**
                             * The basic constructor
                             */
                            tm_source_entry()
                            : m_source_uid(UNDEFINED_PHRASE_ID), m_capacity(0), m_targets(NULL), m_next_idx(0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_source_entry() {
                                if (m_targets != NULL) {
                                    delete[] m_targets;
                                    m_targets = NULL;
                                }
                            }

                            /**
                             * Allows to set the source phrase id
                             * @param s_uid the source phrase id
                             */
                            inline void set_source_uid(phrase_uid s_uid) {
                                m_source_uid = s_uid;
                            }

                            /**
                             * Should be called to start the source entry, i.e. initialize the memory
                             * @param capacity the number of translations for this entry
                             */
                            inline void begin(const size_t capacity) {
                                //Store the number of translation entries
                                m_capacity = capacity;
                                //Instantiate the translations array
                                m_targets = new tm_target_entry[m_capacity]();
                            }

                            /**
                             * Should be called to indicate that this source entry is finished, i.e. all the translations have been set.
                             */
                            inline void finalize() {
                                //Nothing to be done at the moment
                                ASSERT_SANITY_THROW((m_next_idx != m_capacity),
                                        string("The number of translations in the source entry: ") +
                                        to_string(m_next_idx) + string("is not equal to expected: ") +
                                        to_string(m_capacity));
                            }

                            /**
                             * Allows to add a new translation to the source entry for the given target phrase
                             * @param target the target phrase string 
                             * @param target_uid the uid of the target phrase
                             * @param num_features the number of features in the next array
                             * @param weights the features to put into the entry
                             */
                            inline void add_translation(const string & target, const phrase_uid target_uid,
                                    const size_t num_features, const feature_array features) {
                                //Perform a sanity check
                                ASSERT_SANITY_THROW((m_next_idx >= m_capacity),
                                        string("Exceeding the source entry capacity: ") + to_string(m_capacity));
                                
                                //Get the next free entry for the target phrase
                                tm_target_entry & entry = m_targets[m_next_idx++];

                                //Copy the array memory
                                entry.set_features(num_features, features);

                                //Set the entry's target phrase and its id
                                entry.set_source_target(m_source_uid, target, target_uid);
                            }

                            /**
                             * Allows to check if the translation of the given target is present.
                             * NOTE: This check is not optimal a better data structure for storing
                             * entries might be needed, although this method is only used when
                             * building the Reordering model.
                             * @param target_uid the unique identifier of the taret
                             * @return true if the target is known, otherwise false
                             */
                            bool has_translation(const phrase_uid target_uid) const {
                                LOG_DEBUG1 << "Checking for a source/target (" << m_source_uid
                                        << "/" << target_uid << ") pair entry " << END_LOG;

                                //Compute the source/target uid
                                const phrase_uid m_st_uid = combine_phrase_uids(m_source_uid, target_uid);

                                LOG_DEBUG1 << "The source/target id is " << m_st_uid << END_LOG;

                                //Search for the uid in the array
                                for (size_t idx = 0; idx < m_capacity; ++idx) {
                                    LOG_DEBUG1 << "The source/target translation for "
                                            << m_st_uid << " is found!" << END_LOG;
                                    if (m_targets[idx].get_st_uid() == m_st_uid) {
                                        LOG_DEBUG1 << "The source/target translation for "
                                                << m_st_uid << " is found!" << END_LOG;

                                        return true;
                                    }
                                }

                                LOG_DEBUG1 << "The source/target translation for "
                                        << m_st_uid << " is not found!" << END_LOG;

                                return false;
                            }

                            /**
                             * Allows to get all the source/target phrase identifiers
                             * for the source target translation in this query.
                             * @param st_uids the container for the source/target phrase identifiers
                             */
                            inline void get_st_uids(vector<phrase_uid> & st_uids) const {
                                for (size_t idx = 0; idx < m_capacity; ++idx) {
                                    st_uids.push_back(m_targets[idx].get_st_uid());
                                }
                            }

                            /**
                             * The comparison operator, allows to compare source entries
                             * @param phrase_uid the unique identifier of the source entry to compare with
                             * @return true if the provided uid is equal to the uid of this entry, otherwise false 
                             */
                            inline bool operator==(const phrase_uid & phrase_uid) const {
                                return (m_source_uid == phrase_uid);
                            }

                        private:
                            //Stores the unique identifier of the given source
                            phrase_uid m_source_uid;
                            //Stores the number of translation entries
                            size_t m_capacity;
                            //Stores the target entries array pointer
                            tm_target_entry * m_targets;
                            //Stores the next index for the translation entry
                            size_t m_next_idx;
                        };

                        //Define the pointer to the const source entry
                        typedef const tm_source_entry * tm_const_source_entry_ptr;

                        //Define the pointer to the source entry
                        typedef tm_source_entry * tm_source_entry_ptr;
                    }
                }
            }
        }
    }
}

#endif /* TM_SOURCE_ENTRY_HPP */

