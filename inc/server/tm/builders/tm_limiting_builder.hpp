/* 
 * File:   tm_limiting_builder.hpp
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
 * Created on May 4, 2016, 12:02 PM
 */

#ifndef TM_LIMITING_BUILDER_HPP
#define TM_LIMITING_BUILDER_HPP

#include <cmath>
#include <unordered_map>

#include "tm_builder.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/string_utils.hpp"
#include "common/utils/containers/ordered_list.hpp"

#include "server/server_consts.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/lm/proxy/lm_fast_query_proxy.hpp"
#include "server/lm/lm_configurator.hpp"

#include "server/tm/tm_parameters.hpp"
#include "server/tm/models/tm_target_entry.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace builders {
                        //The typedef for the list of targets
                        typedef ordered_list<tm_target_entry> targets_list;
                        //The typedef for the pointer to the list of targets
                        typedef ordered_list<tm_target_entry>* targets_list_ptr;
                        //Define the map storing the source phrase ids and the number of translations per phrase
                        typedef unordered_map<phrase_uid, targets_list_ptr> tm_data_map;

                        /**
                         * This class represents a builder of the translation model.
                         * It allows to read a text-formatted translation model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The translation model is also commonly known as a phrase table.
                         */
                        template< typename model_type, typename reader_type>
                        class tm_limiting_builder {
                        public:

                            /**
                             * The basic constructor of the builder object
                             * @params params the model parameters
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            tm_limiting_builder(const tm_parameters & params, model_type & model, reader_type & reader)
                            : m_params(params), m_data(), m_model(model), m_reader(reader),
                            m_lm_query(lm_configurator::allocate_fast_query_proxy()), m_tmp_num_words(0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_limiting_builder() {
                                //Dispose the query proxy
                                lm_configurator::dispose_fast_query_proxy(m_lm_query);
                            }

                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
                             * of distinct source phrases.
                             */
                            void build() {
                                //Load the model data into memory and filter
                                load_tm_data();

                                //Convert the loaded data into the
                                //model and delete the temporary data
                                convert_tm_data();

                                //Add the unk (unknown translation) entry
                                add_unk_translation();
                            }

                        protected:

                            /**
                             * Allows to post-process a single feature, i.e. do:
                             *         log10(feature)*lambda
                             * @param feature the feature to post-process
                             * @param lambda the lambda weight to multiply the log10 feature with
                             * @return the post-processed feature
                             */
                            inline float post_process_feature(const float feature, const float lambda) {
                                //Now convert to the log probability and multiply with the appropriate weight
                                float result = log10(feature) * lambda;
                                LOG_DEBUG << "log10(" << feature << ") * " << lambda << " = " << result << END_LOG;
                                return result;
                            }

                            /**
                             * Allows to add an unk entry to the model
                             */
                            inline void add_unk_translation() {
                                //Declare an array of features, zero-valued
                                feature_array unk_features = {};

                                LOG_DEBUG << "The UNK initial features: "
                                        << array_to_string<prob_weight>(m_params.m_num_unk_features, m_params.m_unk_features) << END_LOG;
                                LOG_DEBUG << "The UNK lambdas: "
                                        << array_to_string<prob_weight>(m_params.m_num_unk_features, m_params.m_lambdas) << END_LOG;

                                //Copy the values of the unk features to the writable array
                                for (size_t idx = 0; idx < m_params.m_num_unk_features; ++idx) {
                                    //Now convert to the log probability and multiply with the appropriate weight
                                    unk_features[idx] = post_process_feature(m_params.m_unk_features[idx], m_params.m_lambdas[idx]);
                                }

                                //Set the unk features to the model
                                m_model.set_unk_entry(UNKNOWN_WORD_ID, m_params.m_num_unk_features,
                                        unk_features, m_lm_query.get_unk_word_prob());
                            }

                            /**
                             * Allows to extract the features from the text piece and to
                             * check that they are valid with respect to the option bound
                             * If needed the weights will be converted to log scale and
                             * multiplied with the lambda factors
                             * @param weights [in] the text piece with weights, that starts with a space!
                             * @param num_features [out] the number of read features if they satisfy on the constraints
                             * @param storage [out] the read and post-processed features features if they satisfy on the constraints
                             * @return true if the features satisfy the constraints, otherwise false
                             */
                            inline bool process_features(text_piece_reader weights, size_t & num_features, prob_weight * storage) {
                                //Declare the token
                                text_piece_reader token;
                                //Store the read probability weight
                                size_t idx = 0;
                                //Store the read weight value
                                float feature;

                                LOG_DEBUG << "Reading the features from: " << weights << END_LOG;

                                //Skip the first space
                                weights.get_first_space(token);

                                LOG_DEBUG3 << "TM features to parse: " << weights << END_LOG;

                                //Read the subsequent weights, check that the number of weights is as expected
                                while (weights.get_first_space(token) && (idx < tm_target_entry::NUM_FEATURES)) {
                                    //Parse the token into the entry weight
                                    ASSERT_CONDITION_THROW(!fast_s_to_f(feature, token.str().c_str()),
                                            string("Could not parse the token: ") + token.str());

                                    LOG_DEBUG3 << "parsed: " << token << " -> " << feature << END_LOG;

                                    //Check the probabilities at the indexes for the bound
                                    if (((idx == 0) || (idx == 2)) && (feature < m_params.m_min_tran_prob)) {
                                        LOG_DEBUG1 << "The feature[" << idx << "] = " << feature
                                                << " < " << m_params.m_min_tran_prob << END_LOG;
                                        return false;
                                    } else {
                                        //Now convert to the log probability and multiply with the appropriate weight
                                        storage[idx] = post_process_feature(feature, m_params.m_lambdas[idx]);
                                    }

                                    //Increment the index 
                                    ++idx;
                                }

                                //Update the number of features
                                num_features = idx;

                                LOG_DEBUG1 << "Got " << num_features << " good features!" << END_LOG;

                                return true;
                            }

                            /**
                             * Allows to create a new target entry in case the 
                             * @param rest the text piece reader to parse the target entry from
                             * @param source_uid the if of the source phrase
                             * @param entry [out] the reference to the entry pointer
                             * @return true if the entry was created, otherwise false
                             */
                            inline bool get_target_entry(text_piece_reader &rest, phrase_uid source_uid, tm_target_entry *&entry) {
                                LOG_DEBUG2 << "Got translation line to parse: ___" << rest << "___" << END_LOG;

                                //Declare an array of weights for temporary use
                                feature_array tmp_features;
                                size_t tmp_features_size = 0;

                                //Declare the target entry storing reader
                                text_piece_reader target;

                                //Skip the first space symbol that follows the delimiter with the source
                                rest.get_first_space(target);

                                //Read the target phrase, it is surrounded by spaces
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                //Check that the weights are good and retrieve them if they are
                                if (process_features(rest, tmp_features_size, tmp_features)) {
                                    LOG_DEBUG2 << "The target phrase is: " << target << END_LOG;
                                    //Add the translation entry to the model
                                    string target_str = target.str();
                                    trim(target_str);
                                    //Compute the target phrase and its uid
                                    const phrase_uid target_uid = get_phrase_uid<true>(target_str);

                                    //Use the language model to get the target translation word ids
                                    m_lm_query.get_word_ids(target, m_tmp_num_words, m_tmp_word_ids);

                                    LOG_DEBUG << "The phrase: ___" << target << "__ got "
                                            << m_tmp_num_words << " word ids: " <<
                                            array_to_string<word_uid>(m_tmp_num_words, m_tmp_word_ids) << END_LOG;

                                    //Initiate a new target entry
                                    entry = new tm_target_entry();
                                    entry->set_data(source_uid, target_str, target_uid,
                                            tmp_features_size, tmp_features,
                                            m_tmp_num_words, m_tmp_word_ids);

                                    LOG_DEBUG1 << "The source/target (" << source_uid
                                            << "/" << target_uid << ") entry was created!" << END_LOG;

                                    return true;
                                } else {
                                    LOG_DEBUG1 << "The target entry for the source "
                                            << source_uid << " was filtered out!" << END_LOG;
                                    return false;
                                }
                            }

                            /**
                             * Parses the tm file and loads the data
                             */
                            inline void parse_tm_file() {
                                //Declare the text piece reader for storing the read line and source phrase
                                text_piece_reader line, source;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;

                                //The pointers to the current targets list
                                targets_list_ptr targets = NULL;
                                //Declare the pointer variable for the target entry
                                tm_target_entry * entry = NULL;

                                LOG_DEBUG << "Start parsing the TM file" << END_LOG;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uid
                                    string next_source_str = source.str();
                                    trim(next_source_str);

                                    LOG_DEBUG << "Got the source phrase: ___" << next_source_str << "___" << END_LOG;

                                    //If we are now reading the new source entry
                                    if (source_str != next_source_str) {
                                        //Delete the unneeded source entry
                                        if (targets != NULL && (targets->get_size() == 0)) {
                                            LOG_DEBUG << "Deleting the previous source entry "
                                                    << source_uid << ", as it has no targets!" << END_LOG;
                                            delete targets;
                                            m_data.erase(source_uid);
                                        }

                                        //Store the new source string
                                        source_str = next_source_str;
                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);

                                        LOG_DEBUG1 << "The NEW source ___" << source_str << "___ id is: " << source_uid << END_LOG;

                                        //Create a new list of targets
                                        targets = new targets_list(m_params.m_trans_limit);
                                        m_data[source_uid] = targets;
                                    }

                                    //Get the target entry if it is passes
                                    if (get_target_entry(line, source_uid, entry)) {
                                        LOG_DEBUG1 << "Adding the new target entry to the source " << source_uid << END_LOG;
                                        //Add the translation entry to the list
                                        targets->add_elemenent(entry);
                                    }
                                    LOG_DEBUG1 << "The source " << source_uid << " targets count is " << targets->get_size() << END_LOG;

                                    //Update the progress bar status
                                    logger::update_progress_bar();
                                }
                            }

                            /**
                             * Loads the translation model data into memory and filters is
                             */
                            inline void load_tm_data() {
                                logger::start_progress_bar(string("Pre-loading the phrase translations"));

                                //Count the good entries
                                parse_tm_file();

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();

                                LOG_INFO << "The number of loaded TM source entries is: " << m_data.size() << END_LOG;
                            }

                            /**
                             * Loads the translation model data into memory and filters is
                             */
                            inline void convert_tm_data() {
                                logger::start_progress_bar(string("Storing the pre-loaded phrase translations"));

                                //Set the number of entries into the model
                                m_model.set_num_entries(m_data.size());

                                //Iterate through the map elements and do conversion
                                for (tm_data_map::iterator it = m_data.begin(); it != m_data.end(); ++it) {
                                    //The current source id
                                    phrase_uid source_uid = it->first;
                                    //The pointers to the current targets list
                                    targets_list_ptr targets = it->second;

                                    //Add the source entry and the translation entries
                                    if (targets->get_size() != 0) {
                                        //Open the new source entry
                                        tm_source_entry * source_entry = m_model.begin_entry(source_uid, targets->get_size());

                                        //The pointer variable for the target entry container
                                        ordered_list<tm_target_entry>::elem_container * entry = targets->get_first();

                                        //Add the translation entries
                                        while (entry != NULL) {
                                            //Get the reference to the target entry
                                            tm_target_entry & target = **entry;

                                            //Get the language model weights for the target translation
                                            const prob_weight lm_weight = m_lm_query.execute(target.get_num_words(), target.get_word_ids());
                                            LOG_DEBUG << "The phrase: ___" << target.get_target_phrase() << "__ lm-weight: " << lm_weight << END_LOG;

                                            source_entry->add_target(target, lm_weight);

                                            //Move on to the next entry
                                            entry = entry->m_next;
                                        }

                                        //Finalize the source entry
                                        m_model.finalize_entry(source_uid);
                                    }

                                    //Delete the entry
                                    delete targets;
                                }
                                
                                //Erase the map entries
                                m_data.clear();

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();

                                LOG_INFO << "The phrase-translations table is created and loaded" << END_LOG;
                            }


                        private:
                            //Stores the reference to the model parameters
                            const tm_parameters & m_params;

                            //The map storing the model sizes
                            tm_data_map m_data;

                            //Stores the reference to the model
                            model_type & m_model;

                            //Stores the reference to the builder;
                            reader_type & m_reader;

                            //Stores the reference to the LM query proxy
                            lm_fast_query_proxy & m_lm_query;

                            //The temporary variable to store the number of words in the target translation phrase
                            phrase_length m_tmp_num_words;
                            //The temporary variable to store word ids for the target translation phrase LM word ids
                            word_uid m_tmp_word_ids[TM_MAX_TARGET_PHRASE_LEN];
                        };
                    };
                }
            }
        }
    }
}

#endif /* TM_LIMITING_BUILDER_HPP */

