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
#include <map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/rm/rm_parameters.hpp"

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
#if IS_SERVER_TUNING_MODE
                                memset(m_pure_weights, 0, NUM_FEATURES * sizeof (prob_weight));
#endif
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_entry_temp() {
                            }

                            /**
                             * Allows to get the entry weights array
                             * @return the entry weights array
                             */
                            inline const prob_weight * get_weights() const {
                                return m_weights;
                            }

                            /**
                             * Allows to get the weight for the given distortion value
                             * @param is_from the flag allowing to distinguish between the from and to case 
                             * if true then we get the value from the from source phrase case
                             * if false then we get the value for the to source phrase case
                             * @param orient the reordering orientation
                             * @param scores [in/out] the pointer to the map storing the mapping from the feature name
                             *               to the feature score, without lambda. If not null and we are in the tuning
                             *               mode the map will be filled in with the feature-value data
                             * @return the weight for the given distortion value
                             */
                            template<bool is_from>
                            inline const prob_weight get_weight(const reordering_orientation orient, map<string, prob_weight> * scores = NULL) const {
                                //Compute the static position correction for the from/to cases
                                static constexpr uint32_t pos_corr = (is_from ? 0 : num_features / 2);
                                static constexpr uint32_t mon_pos = pos_corr;
                                static constexpr uint32_t swap_pos = ((num_features <= TWO_RM_FEATURES) ? mon_pos : mon_pos + 1);
                                static constexpr uint32_t disc_left_pos = ((num_features <= FOUR_RM_FEATURES) ? swap_pos : swap_pos + 1);
                                static constexpr uint32_t disc_right_pos = ((num_features <= SIX_RM_FEATURES) ? disc_left_pos : disc_left_pos + 1);

                                LOG_DEBUG << (is_from ? "from mon_pos = " : "to mon_pos = ") << to_string(mon_pos)
                                        << ", swap_pos = " << to_string(swap_pos) << ", disc_left_pos =" << to_string(disc_left_pos)
                                        << ", disc_right_pos = " << to_string(disc_right_pos) << END_LOG;

                                //Return the proper weight based on the orientation
                                switch (orient) {
                                    case reordering_orientation::MONOTONE_ORIENT:
                                        LOG_DEBUG1 << "MONOTONE_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << mon_pos << ", value: " << m_weights[mon_pos] << END_LOG;
#if IS_SERVER_TUNING_MODE
                                        LOG_DEBUG1 << mon_pos << " -> " << rm_parameters::RM_WEIGHT_NAMES[mon_pos] << END_LOG;
                                        if (scores != NULL) scores->operator[](rm_parameters::RM_WEIGHT_NAMES[mon_pos]) = m_pure_weights[mon_pos];
#endif
                                        return m_weights[mon_pos];
                                    case reordering_orientation::SWAP_ORIENT:
                                        LOG_DEBUG1 << "SWAP_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << swap_pos << ", value: " << m_weights[swap_pos] << END_LOG;
#if IS_SERVER_TUNING_MODE
                                        LOG_DEBUG1 << swap_pos << " -> " << rm_parameters::RM_WEIGHT_NAMES[swap_pos] << END_LOG;
                                        if (scores != NULL) scores->operator[](rm_parameters::RM_WEIGHT_NAMES[swap_pos]) = m_pure_weights[swap_pos];
#endif
                                        return m_weights[swap_pos];
                                    case reordering_orientation::DISCONT_RIGHT_ORIENT:
                                        LOG_DEBUG1 << "DISCONT_RIGHT_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << disc_left_pos << ", value: " << m_weights[disc_left_pos] << END_LOG;
#if IS_SERVER_TUNING_MODE
                                        LOG_DEBUG1 << disc_left_pos << " -> " << rm_parameters::RM_WEIGHT_NAMES[disc_left_pos] << END_LOG;
                                        if (scores != NULL) scores->operator[](rm_parameters::RM_WEIGHT_NAMES[disc_left_pos]) = m_pure_weights[disc_left_pos];
#endif
                                        return m_weights[disc_left_pos];
                                    case reordering_orientation::DISCONT_LEFT_ORIENT:
                                        LOG_DEBUG1 << "DISCONT_LEFT_ORIENT " << (is_from ? "'from'" : "'to'")
                                                << " position: " << disc_right_pos << ", value: " << m_weights[disc_right_pos] << END_LOG;
#if IS_SERVER_TUNING_MODE
                                        LOG_DEBUG1 << disc_right_pos << " -> " << rm_parameters::RM_WEIGHT_NAMES[disc_right_pos] << END_LOG;
                                        if (scores != NULL) scores->operator[](rm_parameters::RM_WEIGHT_NAMES[disc_right_pos]) = m_pure_weights[disc_right_pos];
#endif
                                        return m_weights[disc_right_pos];
                                    default:
                                        THROW_EXCEPTION(string("Unsupported orientation value: ") + to_string(orient));
                                }
                            }

                            /**
                             * Allows to set the probability weight inside the entry.
                             * This should be the log_10 scale probability weight
                             * @param idx the feature index
                             * @param weight the log_10 weight of the feature, without lambda
                             * @param lambda the lambda to multiply the weight with
                             */
                            inline void set_weight(const size_t idx, const prob_weight weight, const prob_weight lambda) {
#if IS_SERVER_TUNING_MODE
                                m_pure_weights[idx] = weight;
#endif
                                m_weights[idx] = weight * lambda;
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

                            /**
                             * Allows to compute the reordering orientation for the phrase based lexicolized reordering model
                             * @param prev_begin_word_idx the previous translated phrase begin word index
                             * @param prev_end_word_idx the previous translated phrase end word index
                             * @param next_begin_word_idx the next translated phrase begin word index
                             * @param next_end_word_idx the next translated phrase end word index
                             * @return the reordering orientation
                             */
                            static inline reordering_orientation get_reordering_orientation(
                                    const int32_t & prev_begin_word_idx, const int32_t & prev_end_word_idx,
                                    const int32_t & next_begin_word_idx, const int32_t & next_end_word_idx) {
                                LOG_DEBUG2 << "Previous state end_word_idx: " << prev_end_word_idx
                                        << ", new state begin_word_idx: " << next_begin_word_idx << END_LOG;

                                if (next_begin_word_idx < prev_end_word_idx) {
                                    //We went to the left from the previous translation
                                    if ((prev_begin_word_idx - next_end_word_idx) == 1) {
                                        LOG_DEBUG2 << "SWAP_ORIENT" << END_LOG;
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::SWAP_ORIENT;
                                    } else {
                                        LOG_DEBUG2 << "DISCONT_LEFT_ORIENT" << END_LOG;
                                        //We have a discontinuous jump from the previous
                                        return reordering_orientation::DISCONT_LEFT_ORIENT;
                                    }
                                } else {
                                    //We went to the right from the previous translation
                                    if ((next_begin_word_idx - prev_end_word_idx) == 1) {
                                        LOG_DEBUG2 << "MONOTONE_ORIENT" << END_LOG;
                                        //The current phrase is right next to the previous
                                        return reordering_orientation::MONOTONE_ORIENT;
                                    } else {
                                        LOG_DEBUG2 << "DISCONT_RIGHT_ORIENT" << END_LOG;
                                        //We have a discontinuous jump from the previous
                                        return reordering_orientation::DISCONT_RIGHT_ORIENT;
                                    }
                                }
                            }

                        private:
                            //Stores the phrase id, i.e. the unique identifier for the source/target phrase pair
                            phrase_uid m_uid;
                            //This is an array of reordering weights
                            prob_weight m_weights[NUM_FEATURES];
#if IS_SERVER_TUNING_MODE
                            //This is an array of reordering weights not multiplied with lambda's
                            prob_weight m_pure_weights[NUM_FEATURES];
#endif

                            //Add a friend operator for easy output
                            template<uint8_t num_weights>
                            friend ostream & operator<<(ostream & stream, const rm_entry_temp<num_weights> & entry);
                        };

                        template<uint8_t num_features>
                        constexpr uint8_t rm_entry_temp<num_features>::NUM_FEATURES;

                        //Instantiate template
                        typedef rm_entry_temp<NUM_RM_FEATURES> rm_entry;

                        /**
                         * This operator allows to stream the reordering entry to the output stream
                         * @param stream the stream to send the data into
                         * @param entry the entry to stream
                         * @return the reference to the same stream is returned
                         */
                        template<uint8_t num_weights>
                        static inline ostream & operator<<(ostream & stream, const rm_entry_temp<num_weights> & entry) {
                            return stream << "[ uid: " << entry.m_uid << ", weights: "
                                    << array_to_string<prob_weight>(rm_entry::NUM_FEATURES, entry.m_weights) << " ]";
                        }
                    }
                }
            }
        }
    }
}

#endif /* RM_ENTRY_HPP */

