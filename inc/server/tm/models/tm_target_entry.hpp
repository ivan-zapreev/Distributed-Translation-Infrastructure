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
                        class tm_target_entry {
                        public:
                            //Define the variable storing the unknown target uid
                            static const phrase_uid UNKNOWN_TARGET_ENTRY_UID;

                            /**
                             * The basic constructor
                             */
                            tm_target_entry()
                            : m_target_phrase(""), m_num_words(0), m_word_ids(NULL), m_st_uid(UNDEFINED_PHRASE_ID),
                            m_t_cond_s(UNKNOWN_LOG_PROB_WEIGHT), m_total_weight(UNKNOWN_LOG_PROB_WEIGHT) {
                                //Check that the number of features is set
                                ASSERT_SANITY_THROW((NUMBER_OF_FEATURES == 0),
                                        "The NUMBER_OF_FEATURES has not been set!");
#if IS_SERVER_TUNING_MODE
                                m_pure_features = NULL;
#endif                        
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_target_entry() {
                                //Deallocate the word ids
                                if (m_word_ids != NULL) {
                                    delete[] m_word_ids;
                                    m_word_ids = NULL;
                                }
#if IS_SERVER_TUNING_MODE
                                if (m_pure_features != NULL) {
                                    delete m_pure_features;
                                    m_pure_features = NULL;
                                }
#endif
                            }

                            /**
                             * Allows to compare two target entries based on their total weight
                             * @param other the other entry to compare with
                             * @return true if the total weight of this entry is smaller than the total weight of the other entry
                             */
                            inline bool operator<(const tm_target_entry & other) const {
                                return get_total_weight<false>() < other.get_total_weight<false>();
                            }

                            /**
                             * Allows to set the target phrase and its id
                             * @param source_uid store the source uid for being combined with the
                             *                   target phrase into the source/target pair uid
                             * @param target_phrase the target phrase
                             * @param target_uid the uid of the target phrase
                             * @param features the weights to be set into the entry
                             * @param num_words the number of words in the target translation
                             * @param word_ids the LM word ids for the target phrase 
                             * @param pure_features the feature values without the lambda weights,
                             *        to be stored for server tuning mode, default is NULL
                             */
                            inline void set_data(const phrase_uid source_uid,
                                    const string & target_phrase, const phrase_uid target_uid,
                                    const prob_weight * features, const phrase_length num_words,
                                    const word_uid * word_ids, const prob_weight * pure_features = NULL) {
                                //Store the target phrase
                                m_target_phrase = target_phrase;

                                //Store the number of words and the corresponding word ids
                                m_num_words = num_words;
                                m_word_ids = new word_uid[m_num_words];
                                memcpy(m_word_ids, word_ids, m_num_words * sizeof (word_uid));

                                //Compute and store the source/target phrase uid
                                m_st_uid = combine_phrase_uids(source_uid, target_uid);

                                //Set the features 
                                set_features(features, pure_features);

                                LOG_DEBUG1 << "Adding the source/target (" << source_uid << "/"
                                        << target_uid << ") entry with id" << m_st_uid << END_LOG;
                            }

                            /**
                             * This method allows to move the data from the given target entry to this one.
                             * The dynamically allocated data from the given entry is moved to this one,
                             * the rest is copied.
                             * @param other the other entry to move the values from
                             */
                            inline void move_from(tm_target_entry & other) {
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
                                m_pure_features = other.m_pure_features;
                                //Make sure the features in the target
                                //entry we move from are discarded
                                other.m_pure_features = NULL;
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
                             * of features that are turned into log_e scale.
                             * @param is_consider_scores if true then the scores will be considered in the tuning mode
                             * @param scores the pointer to the array of feature scores that is to 
                             *               be filled in, unless the provided pointer is NULL.
                             * @return the total weight of the entry, the sum of feature weights
                             */
                            template<bool is_consider_scores = true>
                            inline const prob_weight get_total_weight(prob_weight * scores = NULL) const {
#if IS_SERVER_TUNING_MODE
                                if (is_consider_scores) {
                                    ASSERT_SANITY_THROW((NUMBER_OF_FEATURES == 0), string("The number of features is zero!"));
                                    LOG_DEBUG1 << this << ": The features: "
                                            << array_to_string<prob_weight>(NUMBER_OF_FEATURES, m_pure_features) << END_LOG;
                                    ASSERT_SANITY_THROW((scores == NULL), string("The scores pointer is NULL!"));
                                    for (int8_t idx = 0; idx != NUMBER_OF_FEATURES; ++idx) {
                                        scores[tm_parameters::TM_WEIGHT_GLOBAL_IDS[idx]] = m_pure_features[idx];
                                        LOG_DEBUG2 << tm_parameters::TM_WEIGHT_NAMES[idx] << " = " << m_pure_features[idx] << END_LOG;
                                    }
                                }
#endif
                                return m_total_weight;
                            }

                            /**
                             * Allows to get the value of the third feature which is the log_e(p(e|f))
                             * @return  the value of the third feature which is the log_e(p(e|f))
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
                                LOG_DEBUG1 << "Setting the number of TM features: " << to_string(num_features) << END_LOG;

                                ASSERT_CONDITION_THROW((num_features <= 0),
                                        string("The number of features: ") + to_string(num_features) +
                                        string(" must be a positive value!"));

                                //Store the number of features
                                NUMBER_OF_FEATURES = num_features;
                            }

                        protected:

                            /**
                             * Allows to set the weights into the target entry.
                             * \todo Get rid of magic constants in this function!
                             * @param features the weights to be set into the entry
                             * @param pure_features the feature values without the lambda weights,
                             *        to be stored for server tuning mode, default is NULL
                             * This is an array of translation weights, as we have here:
                             * features[0] = p(f|e);
                             * features[1] = lex(p(f|e));
                             * features[2] = p(e|f);
                             * features[3] = lex(p(e|f));
                             * features[4] = phrase penalty;
                             */
                            inline void set_features(const prob_weight * features, const prob_weight * pure_features = NULL) {
#if IS_SERVER_TUNING_MODE
                                //Check that the pure features list is present
                                ASSERT_SANITY_THROW((pure_features == NULL), "The pure_features is NULL!");

                                //Allocate the features storage
                                m_pure_features = new prob_weight[NUMBER_OF_FEATURES]();
                                //Store the individual feature weights
                                memcpy(m_pure_features, pure_features, sizeof (prob_weight) * NUMBER_OF_FEATURES);

                                LOG_DEBUG1 << this << ": The features: "
                                        << array_to_string<prob_weight>(NUMBER_OF_FEATURES, m_pure_features) << END_LOG;
#endif

                                //Compute the total weight
                                m_total_weight = 0.0; //First re-set to zero
                                for (int8_t idx = 0; idx < NUMBER_OF_FEATURES; ++idx) {
                                    m_total_weight += features[idx];
                                }

                                //Check that we have enough features
                                //ToDo: Why 3 and 2, later on? We shall change this into
                                //      a constant and this kind of check is also bogus ...
                                ASSERT_SANITY_THROW((NUMBER_OF_FEATURES < 3),
                                        "The must be at least 3 features, p(e|f) is not known!");

                                //Store the target conditioned on source probability
                                m_t_cond_s = features[2];
                            }

                        private:
                            //Stores the number of weights constant for the reordering entry
                            //This value is initialized before the RM model is loaded
                            static int8_t NUMBER_OF_FEATURES;

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
                            //Stores the the features
                            prob_weight * m_pure_features;
#endif                            
                        };

                        //Define the constant entry
                        typedef const tm_target_entry tm_const_target_entry;

                        //Typedef an array of weights
                        typedef prob_weight feature_array[MAX_NUM_TM_FEATURES];
                    }
                }
            }
        }
    }
}

#endif /* TM_TARGET_ENTRY_HPP */

