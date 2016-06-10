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
                            MONOTONE_ORIENT = 0,
                            SWAP_ORIENT = MONOTONE_ORIENT + 1,
                            DISCONT_LEFT_ORIENT = SWAP_ORIENT + 1,
                            DISCONT_RIGHT_ORIENT = DISCONT_LEFT_ORIENT + 1,
                            UNKNOWN_ORIENT = DISCONT_RIGHT_ORIENT + 1,
                            size = UNKNOWN_ORIENT + 1
                        };

                        /**
                         * This is the reordering entry class it stores the
                         * reordering penalties for one source to target phrase.
                         * @param m_num_features is the number of reordering weights
                         */
                        class rm_entry {
                        public:

                            /**
                             * The basic constructor
                             */
                            rm_entry() : m_uid(UNDEFINED_PHRASE_ID) {
                                //Check that the number of features is set
                                ASSERT_SANITY_THROW((NUMBER_OF_FEATURES == 0),
                                        "The NUMBER_OF_FEATURES has not been set!");

                                m_weights = new prob_weight[NUMBER_OF_FEATURES]();
#if IS_SERVER_TUNING_MODE
                                m_pure_features = new prob_weight[NUMBER_OF_FEATURES]();
#endif
                            }

                            /**
                             * The basic destructor
                             */
                            ~rm_entry() {
                                if (m_weights != NULL) delete[] m_weights;
#if IS_SERVER_TUNING_MODE
                                if (m_pure_features != NULL) delete[] m_pure_features;
#endif
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
                             * @param is_consider_scores if true then the scores will be considered in the tuning mode
                             * if true then we get the value from the from source phrase case
                             * if false then we get the value for the to source phrase case
                             * @param orient the reordering orientation
                             * @param scores the pointer to the array of feature scores that is to 
                             *               be filled in, unless the provided pointer is NULL.
                             * @return the weight for the given distortion value
                             */
                            template<bool is_from, bool is_consider_scores = true >
                            inline const prob_weight get_weight(const reordering_orientation orient, prob_weight * scores = NULL) const {
                                //Get the position of the feature value in the features array
                                const int8_t position = (is_from ? FROM_POSITIONS[orient] : TO_POSITIONS[orient]);
                                LOG_DEBUG2 << (is_from ? "FROM " : "TO ") << "ORIENTATION " << to_string(orient) << " position: "
                                        << to_string(position) << ", value: " << m_weights[position] << END_LOG;

#if IS_SERVER_TUNING_MODE
                                if (is_consider_scores) {
                                    ASSERT_SANITY_THROW((scores == NULL), string("The scores pointer is NULL!"));
                                    //Store the pure feature weight if in the tuning mode 
                                    scores[rm_parameters::RM_WEIGHT_GLOBAL_IDS[position]] = m_pure_features[position];
                                    LOG_DEBUG2 << m_pure_features << "@" << rm_parameters::RM_WEIGHT_NAMES[position] << " = " << m_pure_features[position] << END_LOG;
                                }
#endif

                                //Return the weight value
                                return m_weights[position];
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
                                m_pure_features[idx] = weight;
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
                            inline bool operator==(const rm_entry & other) const {
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

                            /**
                             * Allows to get the number of features
                             * @return the number of features
                             */
                            static size_t get_num_features() {
                                ASSERT_CONDITION_THROW((NUMBER_OF_FEATURES == 0),
                                        string("The number of features has not been set!"));

                                return NUMBER_OF_FEATURES;
                            }

                            /**
                             * Allows to set the number of RM model features, must be called once during program execution
                             * @param num_features the number of features to set
                             */
                            static void set_num_features(const int8_t num_features) {
                                LOG_DEBUG1 << "Setting the number of RM features: " << to_string(num_features) << END_LOG;

                                ASSERT_CONDITION_THROW((num_features <= 0),
                                        string("The number of features: ") + to_string(num_features) +
                                        string(" must be a positive value!"));

                                //Store the number of features
                                NUMBER_OF_FEATURES = num_features;

                                //Compute the number of feature types
                                const int8_t HALF_NUMBER_OF_FEATURES = NUMBER_OF_FEATURES / 2;

                                LOG_DEBUG1 << "Starting to initialize the FROM and TO positions array" << END_LOG;

                                //Compute the from and to position of the monotone move feature
                                FROM_POSITIONS[0] = 0;
                                TO_POSITIONS[0] = HALF_NUMBER_OF_FEATURES;

                                //Initialize the from and to positions in the loop
                                for (int8_t idx = 1; idx < HALF_MAX_NUM_RM_FEATURES; ++idx) {
                                    if (idx < HALF_NUMBER_OF_FEATURES) {
                                        FROM_POSITIONS[idx] = FROM_POSITIONS[idx - 1] + 1;
                                        TO_POSITIONS[idx] = TO_POSITIONS[idx - 1] + 1;
                                    } else {
                                        FROM_POSITIONS[idx] = FROM_POSITIONS[HALF_NUMBER_OF_FEATURES - 1];
                                        TO_POSITIONS[idx] = TO_POSITIONS[HALF_NUMBER_OF_FEATURES - 1];
                                    }
                                }

                                LOG_DEBUG << " FROM_POSITIONS = " << array_to_string(HALF_NUMBER_OF_FEATURES, FROM_POSITIONS)
                                        << ", TO_POSITIONS =" << array_to_string(HALF_NUMBER_OF_FEATURES, TO_POSITIONS) << END_LOG;
                            }

                        private:
                            //Stores the number of weights constant for the reordering entry
                            //This value is initialized before the RM model is loaded
                            static int8_t NUMBER_OF_FEATURES;
                            //Stores the difference move position indexes in the feature array
                            //These values are initialized before the RM model is loaded
                            static int8_t FROM_POSITIONS[HALF_MAX_NUM_RM_FEATURES];
                            static int8_t TO_POSITIONS[HALF_MAX_NUM_RM_FEATURES];

                            //Stores the phrase id, i.e. the unique identifier for the source/target phrase pair
                            phrase_uid m_uid;
                            //This is an array of reordering weights
                            prob_weight * m_weights;
#if IS_SERVER_TUNING_MODE
                            //This is an array of reordering weights not multiplied with lambda's
                            prob_weight * m_pure_features;
#endif

                            //Add a friend operator for easy output
                            friend ostream & operator<<(ostream & stream, const rm_entry & entry);
                        };

                        /**
                         * This operator allows to stream the reordering entry to the output stream
                         * @param stream the stream to send the data into
                         * @param entry the entry to stream
                         * @return the reference to the same stream is returned
                         */
                        ostream & operator<<(ostream & stream, const rm_entry & entry);
                    }
                }
            }
        }
    }
}

#endif /* RM_ENTRY_HPP */

