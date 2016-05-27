/* 
 * File:   tm_target_entry.hpp
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
 * Created on February 9, 2016, 5:35 PM
 */

#ifndef TM_TARGET_ENTRY_HPP
#define TM_TARGET_ENTRY_HPP

#include <cstring>
#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/hashing_utils.hpp"

#include "server/lm/proxy/lm_fast_query_proxy.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/tm/tm_parameters.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::hashing;

using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        /**
                         * This structure represents the translation data, i.e. the
                         * the target phrase plus the probability weights. See:
                         * http://www.statmt.org/moses/?n=FactoredTraining.ScorePhrases
                         * for more details on the weights. Note that for this entry
                         * we have a uid that is a unique identifier of the target
                         * phrase string. The latter can be a hash value but then
                         * there is a possibility for the hash collisions
                         */
                        template<uint8_t max_num_features>
                        class tm_target_entry_temp {
                        public:
                            //Define the number of weights constant for the reordering entry
                            static constexpr uint8_t NUM_FEATURES = max_num_features;

                            //Define the variable storing the unknown target uid
                            static const phrase_uid UNKNOWN_TARGET_ENTRY_UID;

                            /**
                             * The basic constructor
                             */
                            tm_target_entry_temp()
                            : m_target_phrase(""), m_num_words(0), m_word_ids(NULL), m_st_uid(UNDEFINED_PHRASE_ID),
                            m_t_cond_s(UNKNOWN_LOG_PROB_WEIGHT), m_total_weight(UNKNOWN_LOG_PROB_WEIGHT) {
#if IS_SERVER_TUNING_MODE
                                m_num_features = 0;
                                memset(m_pure_features, 0, NUM_FEATURES * sizeof (prob_weight));
#endif                        
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_target_entry_temp() {
                                //Deallocate the word ids
                                if (m_word_ids != NULL) {
                                    delete[] m_word_ids;
                                    m_word_ids = NULL;
                                }
                            }

                            /**
                             * Allows to compare two target entries based on their total weight
                             * @param other the other entry to compare with
                             * @return true if the total weight of this entry is smaller than the total weight of the other entry
                             */
                            inline bool operator<(const tm_target_entry_temp & other) const {
                                return get_total_weight() < other.get_total_weight();
                            }

                            /**
                             * Allows to set the target phrase and its id
                             * @param source_uid store the source uid for being combined with the
                             *                   target phrase into the source/target pair uid
                             * @param target_phrase the target phrase
                             * @param target_uid the uid of the target phrase
                             * @param num_features the number of features to be set, already in the log10 scale
                             * @param features the weights to be set into the entry
                             * @param num_words the number of words in the target translation
                             * @param word_ids the LM word ids for the target phrase 
                             */
                            inline void set_data(const phrase_uid source_uid,
                                    const string & target_phrase, const phrase_uid target_uid,
                                    const size_t num_features, const prob_weight * features,
                                    const phrase_length num_words, const word_uid * word_ids) {
                                //Store the target phrase
                                m_target_phrase = target_phrase;

                                //Store the number of words and the corresponding word ids
                                m_num_words = num_words;
                                m_word_ids = new word_uid[m_num_words];
                                memcpy(m_word_ids, word_ids, m_num_words * sizeof (word_uid));

                                //Compute and store the source/target phrase uid
                                m_st_uid = combine_phrase_uids(source_uid, target_uid);

                                //Set the features 
                                set_features(num_features, features);

                                LOG_DEBUG1 << "Adding the source/target (" << source_uid << "/"
                                        << target_uid << ") entry with id" << m_st_uid << END_LOG;
                            }

                            /**
                             * This method allows to move the data from the given target entry to this one.
                             * The dynamically allocated data from the given entry is moved to this one,
                             * the rest is copied.
                             * @param other the other entry to move the values from
                             */
                            inline void move_from(tm_target_entry_temp & other) {
                                //Copy the target phrase
                                m_target_phrase = other.m_target_phrase;

                                //Copy the number of words in the source phrase and the word ids
                                m_num_words = other.m_num_words;
                                m_word_ids = other.m_word_ids;

                                //Set the word ids to null so that the original
                                //class can not modify/destroy it.
                                other.m_word_ids = NULL;

                                //Copy the source/target uid
                                m_st_uid = other.m_st_uid;
                                //Copy the features[2] = p(e|f);
                                m_t_cond_s = other.m_t_cond_s;
                                //Copy the total weight
                                m_total_weight = other.m_total_weight;

#if IS_SERVER_TUNING_MODE
                                //Copy the features
                                m_num_features = other.m_num_features;
                                memcpy(m_pure_features, other.m_pure_features, sizeof (prob_weight) * m_num_features);
                                //ToDo: Improve performance by making the array dynamically allocated and only copying the pointer
#endif
                            }

                            /**
                             * Allows to check whether this is an unknown translation
                             * @return true if this is UNK translation, otherwise false
                             */
                            bool is_unk_trans() const {
                                return (m_st_uid == UNKNOWN_TARGET_ENTRY_UID);
                            }

                            /**
                             * Allows to get the target phrase
                             * @return the reference to the const target phrase
                             */
                            const string & get_target_phrase() const {
                                return m_target_phrase;
                            }

                            /**
                             * Allows to retrieve the source/target phrase pair uid
                             * @return the source/target phrase pair uid
                             */
                            inline const phrase_uid get_st_uid() const {
                                return m_st_uid;
                            }

                            /**
                             * Allows to get the total weight of the entry, the sum
                             * of features that are turned into log10 scale.
                             * @param scores [in/out] the pointer to the map storing the mapping from the feature name
                             *               to the feature score, without lambda. If not null and we are in the tuning
                             *               mode the map will be filled in with the feature-value data
                             * @return the total weight of the entry, the sum of feature weights
                             */
                            inline const prob_weight get_total_weight(map<string, prob_weight> * scores = NULL) const {
#if IS_SERVER_TUNING_MODE
                                if (scores != NULL) {
                                    ASSERT_CONDITION_THROW((m_num_features == 0), string("The number of features is zero!"));
                                    LOG_DEBUG1 << this << ": The features: "
                                            << array_to_string<prob_weight>(m_num_features, m_pure_features) << END_LOG;
                                    for (size_t idx = 0; idx != m_num_features; ++idx) {
                                        scores->operator[](tm_parameters::TM_WEIGHT_NAMES[idx]) = m_pure_features[idx];
                                        LOG_DEBUG2 << tm_parameters::TM_WEIGHT_NAMES[idx] << " = " << m_pure_features[idx] << END_LOG;
                                    }
                                }
#endif
                                return m_total_weight;
                            }

                            /**
                             * Allows to get the value of the third feature which is the log10(p(e|f))
                             * @return  the value of the third feature which is the log10(p(e|f))
                             */
                            inline const prob_weight get_t_c_s() const {
                                LOG_DEBUG << "m_t_cond_s : " << m_t_cond_s << END_LOG;
                                return m_t_cond_s;
                            }

                            /**
                             * Allows to get the number of words in the target translation
                             * @return the number of words
                             */
                            inline phrase_length get_num_words() const {
                                return m_num_words;
                            }

                            /**
                             * This method allows to get the
                             * @return an array of word ids of the target phrase, the length must be equal to LM_QUERY_LENGTH_MAX
                             */
                            inline const word_uid* get_word_ids() const {
                                return m_word_ids;
                            }

                        protected:

                            /**
                             * Allows to set the weights into the target entry.
                             * \todo Get rid of magic constants here!
                             * @param num_features the number of features to be set, already in the log10 scale
                             * @param features the weights to be set into the entry
                             * This is an array of translation weights, as we have here:
                             * features[0] = p(f|e);
                             * features[1] = lex(p(f|e));
                             * features[2] = p(e|f);
                             * features[3] = lex(p(e|f));
                             * features[4] = phrase penalty;
                             */
                            inline void set_features(const size_t num_features, const prob_weight * features) {
                                ASSERT_CONDITION_THROW((num_features > max_num_features), string("The number of features: ") +
                                        to_string(num_features) + string(" exceeds the maximum: ") + to_string(max_num_features));
                                ASSERT_CONDITION_THROW((num_features == 0), string("The number of features is zero!"));
#if IS_SERVER_TUNING_MODE
                                //Store the number of features
                                m_num_features = num_features;
                                //Store the individual feature weights
                                memcpy(m_pure_features, features, sizeof (prob_weight) * m_num_features);
                                //ToDo: The m_pure_features are to store the weights without lambda!
                                LOG_DEBUG1 << this << ": The features: "
                                        << array_to_string<prob_weight>(m_num_features, m_pure_features) << END_LOG;
#endif

                                //Compute the total weight
                                m_total_weight = 0.0; //First re-set to zero
                                for (size_t idx = 0; idx < num_features; ++idx) {
                                    m_total_weight += features[idx];
                                }

                                //Check that we have enough features
                                ASSERT_SANITY_THROW((num_features < 3),
                                        "The must be at least 3 features, p(e|f) is not known!");

                                //Store the target conditioned on source probability
                                m_t_cond_s = features[2];
                            }

                        private:
                            //Stores the target phrase of the translation which a key value
                            string m_target_phrase;
                            //Stores the number of words in the translation, maximum should be TM_MAX_TARGET_PHRASE_LEN
                            phrase_length m_num_words;
                            //Stores the target phrase Language model word ids 
                            word_uid * m_word_ids;

                            //Stores the source/target phrase id
                            phrase_uid m_st_uid;

                            //Stores the features[2] = p(e|f);
                            prob_weight m_t_cond_s;

                            //Stores the total features weight of the entity
                            prob_weight m_total_weight;

#if IS_SERVER_TUNING_MODE
                            //Stores the number of features
                            size_t m_num_features;
                            //Stores the the features
                            prob_weight m_pure_features[NUM_FEATURES];
#endif                            
                        };

                        template<uint8_t num_features>
                        constexpr uint8_t tm_target_entry_temp<num_features>::NUM_FEATURES;

                        //Instantiate template
                        typedef tm_target_entry_temp<NUM_TM_FEATURES> tm_target_entry;

                        //Define the constant entry
                        typedef const tm_target_entry tm_const_target_entry;

                        //Typedef an array of weights
                        typedef prob_weight feature_array[tm_target_entry::NUM_FEATURES];
                    }
                }
            }
        }
    }
}

#endif /* TM_TARGET_ENTRY_HPP */

