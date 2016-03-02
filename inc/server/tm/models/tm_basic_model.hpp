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

#include "server/lm/proxy/lm_fast_query_proxy.hpp"

#include "server/tm/tm_consts.hpp"
#include "server/tm/models/tm_source_entry.hpp"
#include "server/tm/models/tm_query.hpp"

#include "common/utils/containers/fixed_size_hashmap.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::lm::proxy;

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
                         * result in collisions, the latter is not checked.
                         */
                        class tm_basic_model {
                        public:
                            //Define the translations data map. It represents possible translations for some source phrase.
                            typedef fixed_size_hashmap<tm_source_entry, const phrase_uid &> tm_source_entry_map;

                            /**
                             * The basic class constructor
                             */
                            tm_basic_model() : m_tm_data(NULL), m_unk_entry(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_basic_model() {
                                //Delete the model data if any
                                if (m_tm_data != NULL) {
                                    delete m_tm_data;
                                    m_tm_data = NULL;
                                }

                                //Delete the unk entry if any
                                if (m_unk_entry != NULL) {
                                    delete m_unk_entry;
                                    m_unk_entry = NULL;
                                }
                            }

                            /**
                             * Should be called to add the unk entry to the model
                             * @param unk_word_id the unknown word id from the Language Model
                             * @param num_unk_features the number of initialized unk features
                             * @param unk_features the unk entry features
                             * @param lm_weight the cost of the target (UNK) translation from the LM model
                             */
                            void set_unk_entry(word_uid unk_word_id, const size_t num_unk_features,
                                    feature_array unk_features, const prob_weight lm_weight) {
                                //Initialize the UNK entry
                                m_unk_entry = new tm_source_entry();
                                //Set thew source id
                                m_unk_entry->set_source_uid(UNKNOWN_PHRASE_ID);
                                //Start adding the translations to the entry, there will be just one
                                m_unk_entry->begin(1);

                                //Declare and initialize the word ids array
                                const phrase_length num_words = 1;
                                word_uid word_ids[num_words];
                                word_ids[0] = unk_word_id;

                                //Add the translation entry
                                m_unk_entry->add_target(
                                        tm::TM_UNKNOWN_TARGET_STR, UNKNOWN_PHRASE_ID,
                                        num_unk_features, unk_features,
                                        num_words, word_ids, lm_weight);

                                //Finalize the source entry
                                m_unk_entry->finalize();
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
                             * @param num_entries the number of source phrase entries
                             */
                            inline void set_num_entries(const size_t num_entries) {
                                LOG_DEBUG << "The number of source phrases is: " << num_entries << END_LOG;

                                //Initialize the source entries map
                                m_tm_data = new tm_source_entry_map(__tm_basic_model::SOURCES_BUCKETS_FACTOR, num_entries);
                            }

                            /**
                             * Allows to open a new source entry, i.e. the entry for the new source phrase
                             * @param entry_id the source phrase id for which the entry is to be started
                             * @return the entry associated with the given id
                             */
                            inline tm_source_entry * begin_entry(const phrase_uid entry_id, const size_t num_elems) {
                                LOG_DEBUG1 << "Adding the new source entry for uid: " << entry_id << END_LOG;

                                //Get the new entry from the data storage
                                tm_source_entry & entry = m_tm_data->add_new_element(entry_id);

                                LOG_DEBUG1 << "The source entry for uid: " << entry_id << " is added." << END_LOG;

                                //Set the source phrase id
                                entry.set_source_uid(entry_id);

                                LOG_DEBUG1 << "Initializing the entry: " << entry_id << " with the number of translations." << END_LOG;

                                //Initialize the entry with the number of translations
                                entry.begin(num_elems);

                                LOG_DEBUG1 << "Adding the new source entry for uid: " << entry_id << " - DONE!" << END_LOG;

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
                                LOG_DEBUG1 << "Finiziling the source entry: " << entry_id << END_LOG;

                                //Finish the source entry
                                m_tm_data->get_element(entry_id, entry_id)->finalize();

                                LOG_DEBUG1 << "Finiziling the source entry: " << entry_id << " - DONE!" << END_LOG;
                            }

                            /**
                             * This method is to be called when the translation model is fully read
                             */
                            inline void finalize() {
                                //Nothing to be done here
                            }

                            /**
                             * Allows to get the source entry for the given entry id
                             * @param do_unk if true then if the entry is not present we return UNK
                             *               if false then if the entry is not present we return NULL
                             *               The default value is true.
                             * @param entry_id the source phrase id
                             * @return the source phrase entry or UNK if the entry is not found
                             */
                            template<bool do_unk>
                            tm_const_source_entry * get_source_entry(const phrase_uid entry_id) const {
                                tm_const_source_entry_ptr entry = m_tm_data->get_element(entry_id, entry_id);
                                if (do_unk && (entry == NULL)) {
                                    LOG_DEBUG1 << "Returning the UNK translation for the source uid: " << entry_id << END_LOG;
                                    return m_unk_entry;
                                } else {
                                    LOG_DEBUG1 << "The ptr to the source entry of uid: " << entry_id << " is " << entry << END_LOG;
                                    return entry;
                                }
                            }

                            /**
                             * Allows to check in the given entry is the UNK entry
                             * @param entry the pointer to the entry to be tested
                             * @return true if this is an UNK entry otherwise false
                             */
                            inline bool is_unk_entry(tm_const_source_entry * entry) const {
                                return (entry == m_unk_entry) || (*entry == *m_unk_entry);
                            }

                            /**
                             * Allows to log the model type info
                             */
                            inline void log_model_type_info() const {
                                LOG_USAGE << "Using the hash-based translation model: " << __FILENAME__ << END_LOG;
                            }

                        private:
                            //Stores the translation model data
                            tm_source_entry_map * m_tm_data;
                            //Stores the pointer to the UNK entry
                            tm_source_entry_ptr m_unk_entry;
                        };
                    }
                }
            }
        }
    }
}


#endif /* TM_BASIC_MODEL_HPP */

