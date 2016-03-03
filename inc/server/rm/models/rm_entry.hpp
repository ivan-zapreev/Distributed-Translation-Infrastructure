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
                         * Defined the reordering orientations in the lexicolized model
                         */
                        enum reordering_orientation {
                            UNKNOWN_ORIENT = 0,
                            MONOTONE_ORIENT = UNKNOWN_ORIENT + 1,
                            SWAP_ORIENT = MONOTONE_ORIENT + 1,
                            DISCONT_LEFT_ORIENT = SWAP_ORIENT + 1,
                            DISCONT_RIGHT_ORIENT = DISCONT_LEFT_ORIENT + 1,
                            size = DISCONT_RIGHT_ORIENT + 1
                        };

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
                                memset(m_weights, 0, NUM_FEATURES * sizeof (prob_weight));
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_entry_temp() {
                            }

                            /**
                             * Allows to get the weight for the given distortion value
                             * @param is_from the flag allowing to distinguish between the from and to case 
                             * if true then we get the value from the from source phrase case
                             * if false then we get the value for the to source phrase case
                             * @param orient the reordering orientation
                             * @return the weight for the given distortion value
                             */
                            template<bool is_from>
                            const prob_weight & get_weight(const reordering_orientation orient) const {
                                //Compute the static position correction for the from/to cases
                                static constexpr uint32_t pos_corr = (is_from ? 0 : num_features / 2 );
                                static constexpr uint32_t mon_pos = pos_corr;
                                static constexpr uint32_t swap_pos = ((num_features <= 1) ? mon_pos : mon_pos + 1);
                                static constexpr uint32_t disc_left_pos = ((num_features <= 2) ? swap_pos : swap_pos + 1);
                                static constexpr uint32_t disc_right_pos = ((num_features <= 3) ? disc_left_pos : disc_left_pos + 1);
                                
                                //Return the proper weight based on the orientation
                                switch (orient) {
                                    case reordering_orientation::MONOTONE_ORIENT:
                                        LOG_DEBUG1 << "MONOTONE_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << mon_pos << ", value: " << m_weights[mon_pos] << END_LOG;
                                        return m_weights[mon_pos];
                                    case reordering_orientation::SWAP_ORIENT:
                                        LOG_DEBUG1 << "SWAP_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << swap_pos << ", value: " << m_weights[swap_pos] << END_LOG;
                                        return m_weights[swap_pos];
                                    case reordering_orientation::DISCONT_RIGHT_ORIENT:
                                        LOG_DEBUG1 << "DISCONT_RIGHT_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << disc_left_pos << ", value: " << m_weights[disc_left_pos] << END_LOG;
                                        return m_weights[disc_left_pos];
                                    case reordering_orientation::DISCONT_LEFT_ORIENT:
                                        LOG_DEBUG1 << "DISCONT_LEFT_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << disc_right_pos << ", value: " << m_weights[disc_right_pos] << END_LOG;
                                        return m_weights[disc_right_pos];
                                    default:
                                        THROW_EXCEPTION(string("Unsupported orientation value: ") + to_string(orient));
                                }
                            }

                            /**
                             * This operator allows to work with the given reordering entry weights in an array fashion
                             * @param idx the index of the feature
                             * @return the feature value
                             */
                            inline prob_weight & operator[](size_t idx) {
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

                            /**
                             * The comparison operator, allows to compare entries
                             * @param other the other entry to compare with
                             * @return true if the provided entry has the same uid as this one, otherwise false 
                             */
                            inline bool operator==(const rm_entry_temp & other) const {
                                return (m_uid == other.m_uid);
                            }

                        private:
                            //Stores the phrase id, i.e. the unique identifier for the source/target phrase pair
                            phrase_uid m_uid;
                            //This is an array of reordering weights
                            prob_weight m_weights[num_features];
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

