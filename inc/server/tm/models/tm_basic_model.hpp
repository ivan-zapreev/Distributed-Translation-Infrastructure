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
#include "server/tm/models/tm_entry.hpp"
#include "server/tm/models/tm_query.hpp"

#include "common/utils/containers/fixed_size_hashmap.hpp"

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
                         * This class represents a basic translation model implementation.
                         * The basic model is based on the fixed size hash map which is a
                         * self-implemented linear probing hash map also used in several
                         * tries. This basic model also does not store the phrases as is
                         * but rather the hash values thereof. So it is a hash based
                         * implementation which reduces memory but might occasionally
                         * provide 
                         */
                        class tm_basic_model {
                        public:

                            //Define the translations data map. It represents possible translations for some source phrase.
                            typedef fixed_size_hashmap<tm_source_entry, const phrase_uid &> tm_source_entry_map;

                            /**
                             * The basic class constructor
                             * @param num_source_entries the number of source language phrases
                             */
                            tm_basic_model() : m_trans_data(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_basic_model() {
                                if (m_trans_data != NULL) {
                                    delete m_trans_data;
                                    m_trans_data = NULL;
                                }
                            }

                            /**
                             * This method is needed to set the number of source phrase entries
                             * This is to be done before adding the translation entries to the model
                             * @param num_entries the number of source entries
                             */
                            void set_num_entries(const size_t num_entries) {
                                m_trans_data = new tm_source_entry_map(__tm_basic_model::BUCKETS_FACTOR, num_entries);
                            }

                            /**
                             * Allows to log the model type info
                             */
                            void log_model_type_info() {
                                LOG_USAGE << "Using the hash-based translation model: " << __FILENAME__ << END_LOG;
                            }

                        protected:

                        private:

                            //Stores the translation model data
                            tm_source_entry_map * m_trans_data;

                        };
                    }
                }
            }
        }
    }
}


#endif /* TM_BASIC_MODEL_HPP */

