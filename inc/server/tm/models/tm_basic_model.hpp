/* 
 * File:   tm_basic_model.hpp
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
 * Created on February 8, 2016, 10:01 AM
 */

#ifndef TM_BASIC_MODEL_HPP
#define TM_BASIC_MODEL_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/tm/tm_configs.hpp"
#include "server/tm/models/tm_source_entry.hpp"
#include "server/tm/models/tm_query.hpp"

#include "common/utils/containers/fixed_size_hashmap.hpp"

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
                         * This class represents a basic translation model implementation.
                         * The basic model is based on the fixed size hash map which is a
                         * self-implemented linear probing hash map also used in several
                         * tries. This basic model also does not store the phrases as is
                         * but rather the hash values thereof. So it is a hash based
                         * implementation which reduces memory but might occasionally
                         * provide.
                         */
                        class tm_basic_model {
                        public:
                            //Define the translations data map. It represents possible translations for some source phrase.
                            typedef fixed_size_hashmap<tm_source_entry, const phrase_uid &> tm_source_entry_map;

                            /**
                             * The basic class constructor
                             */
                            tm_basic_model() : m_sizes(NULL), m_tm_data(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_basic_model() {
                                //Finalize the model just in case it is not
                                finalize();
                                
                                //Delete the model data if any
                                if (m_tm_data != NULL) {
                                    delete m_tm_data;
                                    m_tm_data = NULL;
                                }
                            }

                            /**
                             * This method allows to detect if the number of entries
                             * (source phrases) is needed before the translation
                             * entries are being added.
                             * @return true as this model type uses filed-size hash maps 
                             */
                            inline bool is_num_entries_needed() const {
                                return true;
                            }

                            /**
                             * This method is needed to set the number of source phrase entries
                             * This is to be done before adding the translation entries to the model
                             * The memory of the map will be allocated by this class.
                             * @param sizes the map storing the model sizes
                             */
                            inline void set_num_entries(sizes_map * sizes) {
                                //Stores the sizes
                                m_sizes = sizes;
                                
                                LOG_DEBUG << "The number of source phrases is: " << sizes->size() << END_LOG;
                                
                                //Initialize the source entries map
                                m_tm_data = new tm_source_entry_map(__tm_basic_model::SOURCES_BUCKETS_FACTOR, sizes->size());
                            }

                            /**
                             * Allows to open a new source entry, i.e. the entry for the new source phrase
                             * @param entry_id the source phrase id for which the entry is to be started
                             * @return the entry associated with the given id
                             */
                            inline tm_source_entry * begin_entry(const phrase_uid entry_id) {
                                //Get the new entry from the data storage
                                tm_source_entry & entry = m_tm_data->add_new_element(entry_id);

                                //Set the source phrase id
                                entry.set_source_uid(entry_id);

                                //Initialize the entry with the number of translations
                                entry.begin(m_sizes->at(entry_id));

                                //Return the entry pointer
                                return &entry;
                            }

                            /**
                             * Allows to finish an entry with the given id. The process
                             * of finishing might include many things but the purpose of
                             * it is to indicate that the source entry has been fully read.
                             * I.e. all the translations for the given source are processed.
                             * @param entry_id the source phrase id for which the entry is
                             * to be finished.
                             */
                            inline void finalize_entry(const phrase_uid entry_id) {
                                //Finish the source entry
                                m_tm_data->get_element(entry_id, entry_id)->finalize();
                            }

                            /**
                             * This method is to be called when the translation model is fully read
                             */
                            inline void finalize() {
                                //Delete the sizes map as it is not needed any more
                                if (m_sizes != NULL) {
                                    delete m_sizes;
                                    m_sizes = NULL;
                                }
                            }
                            
                            /**
                             * Allows to get the source entry for the given entry id
                             * In case the entry is not present we return NULL.
                             * @param entry_id the source phrase id
                             * @return the source phrase entry or NULL if the source phrase id is not found
                             */
                            const tm_source_entry * get_source_entry(const phrase_uid entry_id) const {
                                return m_tm_data->get_element(entry_id, entry_id);
                            }

                            /**
                             * Allows to log the model type info
                             */
                            inline void log_model_type_info() const {
                                LOG_USAGE << "Using the hash-based translation model: " << __FILENAME__ << END_LOG;
                            }

                        private:
                            //The map storing the model sizes
                            sizes_map * m_sizes;
                            //Stores the translation model data
                            tm_source_entry_map * m_tm_data;

                        };
                    }
                }
            }
        }
    }
}


#endif /* TM_BASIC_MODEL_HPP */

