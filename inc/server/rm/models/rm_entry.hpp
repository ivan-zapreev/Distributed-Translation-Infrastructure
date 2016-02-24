/* 
 * File:   rm_entry.hpp
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
 * Created on February 8, 2016, 10:06 AM
 */

#ifndef RM_ENTRY_HPP
#define RM_ENTRY_HPP

#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/common/models/phrase_uid.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {

                        /**
                         * This is the reordering entry class it stores the
                         * reordering penalties for one source to target phrase.
                         * @param num_features is the number of reordering weights
                         */
                        template<uint8_t num_features>
                        class rm_entry_temp {
                        public:
                            //Define the number of weights constant for the reordering entry
                            static constexpr uint8_t NUM_FEATURES = num_features;

                            /**
                             * The basic constructor
                             */
                            rm_entry_temp() : m_uid(UNDEFINED_PHRASE_ID) {
                                memset(m_weights, 0, NUM_FEATURES * sizeof(float));
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_entry_temp() {
                            }

                            /**
                             * This operator allows to work with the given reordering entry weights in an array fashion
                             * @param idx the index of the feature
                             * @return the feature value
                             */
                            inline float & operator[](size_t idx) {
                                //Chech that the index is within the bounds
                                ASSERT_SANITY_THROW(idx >= num_features, string("The index: ") + to_string(idx) +
                                        string(" is outside the bounds [0, ") + to_string(num_features - 1) + string("]"));
                                
                                //Return the reference to the corresponding weight
                                return m_weights[idx];
                            }

                            /**
                             * Allows to set the unique source target entry identifier
                             * @param uid the unique identifier of the source/target entry
                             */
                            inline void set_entry_uid(const phrase_uid & uid) {
                                m_uid = uid;
                            }

                            /**
                             * The comparison operator, allows to compare entries
                             * @param phrase_uid the unique identifier of the source/target phrase pair entry to compare with
                             * @return true if the provided uid is equal to the uid of this entry, otherwise false 
                             */
                            inline bool operator==(const phrase_uid & uid) const {
                                return (m_uid == uid);
                            }

                        private:
                            //Stores the phrase id, i.e. the unique identifier for the source/target phrase pair
                            phrase_uid m_uid;
                            //This is an array of reordering weights
                            float m_weights[num_features];
                        };
                        
                        template<uint8_t num_features>
                        constexpr uint8_t rm_entry_temp<num_features>::NUM_FEATURES;
                        
                        //Instantiate template
                        typedef rm_entry_temp<MAX_NUM_RM_FEATURES> rm_entry;
                    }
                }
            }
        }
    }
}

#endif /* RM_ENTRY_HPP */

