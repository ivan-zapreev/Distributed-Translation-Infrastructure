/* 
 * File:   tm_builder.hpp
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
 * Created on February 8, 2016, 9:55 AM
 */

#ifndef TM_BUILDER_HPP
#define TM_BUILDER_HPP

#include <cmath>
#include <unordered_map>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/string_utils.hpp"

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

                        //Stores the translation model delimiter character for parsing one line
                        static const char TM_DELIMITER = '|';
                        //Stores the translation model delimiter character cardinality
                        static const size_t TM_DELIMITER_CDTY = 3;

                        //Define the map storing the source phrase ids and the number of translations per phrase
                        typedef unordered_map<phrase_uid, size_t> sizes_map;

                        /**
                         * This class represents a basic reader of the translation model.
                         * It allows to read a text-formatted translation model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The translation model is also commonly known as a phrase table.
                         */
                        template< typename model_type, typename reader_type>
                        class tm_basic_builder {
                        public:

                            /**
                             * The basic constructor of the builder object
                             * @params params the model parameters
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            tm_basic_builder(const tm_parameters & params, model_type & model, reader_type & reader)
                            : m_params(params), m_model(model), m_reader(reader),
                            m_lm_query(lm_configurator::allocate_fast_query_proxy()),
                            m_tmp_num_words(0) {

                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_basic_builder() {
                                //Dispose the query proxy
                                lm_configurator::dispose_fast_query_proxy(m_lm_query);
                            }

                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
                             * of distinct source phrases.
                             */
                            void build() {
                                //Count and set the number of source phrases if needed
                                if (m_model.is_num_entries_needed()) {
                                    count_source_phrases();
                                }

                                //Process the translations
                                process_source_entries();

                                //Add the unk entry
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
                             * Allows to extract the features from the text piece and to
                             * check that they are valid with respect to the option bound
                             * If needed the weights will be converted to log scale and
                             * multiplied with the lambda factors
                             * @param is_get_weights if the weights are to be retrieved or just checked
                             * @param weights [in] the text piece with weights, that starts with a space!
                             * @param num_features [out] the number of read features if they satisfy on the constraints
                             * @param storage [out] the read and post-processed features features if they satisfy on the constraints
                             * @return true if the features satisfy the constraints, otherwise false
                             */
                            template<bool is_get_weights>
                            inline bool process_features(TextPieceReader weights, size_t & num_features, prob_weight * storage) {
                                //Declare the token
                                TextPieceReader token;
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
                                        if (is_get_weights) {
                                            //Now convert to the log probability and multiply with the appropriate weight
                                            storage[idx] = post_process_feature(feature, m_params.m_lambdas[idx]);
                                        }
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
                             * Allows to check if whether the probability weights satisfy the filtering thresholds.
                             * @param rest the part of the source entry containing the target and the weights
                             * @param tmp_features the temporary weights storage
                             * @return true if the conditions are satisfied, otherwise false
                             */
                            inline bool is_good_features(TextPieceReader rest, size_t & tmp_features_size, prob_weight * tmp_features) {
                                TextPieceReader target;

                                //Skip the target phrase with its end delimiter
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                //Process the weights
                                return process_features<false>(rest, tmp_features_size, tmp_features);
                            }

                            /**
                             * The line format assumes source to target and then at least four weights as given by:
                             *     http://www.statmt.org/moses/?n=FactoredTraining.ScorePhrases
                             * Currently, four different phrase translation scores are computed:
                             *    inverse phrase translation probability φ(f|e)
                             *    inverse lexical weighting lex(f|e)
                             *    direct phrase translation probability φ(e|f)
                             *    direct lexical weighting lex(e|f) 
                             * Previously, there was another score:
                             *    phrase penalty (always exp(1) = 2.718) 
                             * The latter is considered optional, all the other elements
                             * followed on the translation line are now skipped.
                             * @param source_entry the pointer to the source entry for which this translation is
                             * @param rest stores the line to be parsed into a translation entry
                             * @param count_ref [in/out] the number of remaining entries
                             * @param tmp_features_size [out] the number of read features
                             * @param tmp_features the temporary feature storage
                             */
                            inline void process_target_entry(tm_source_entry * source_entry, TextPieceReader &rest,
                                    size_t & count_ref, size_t & tmp_features_size, prob_weight * tmp_features) {
                                LOG_DEBUG2 << "Got translation line to parse: ___" << rest << "___" << END_LOG;

                                //Declare the target entry storing reader
                                TextPieceReader target;

                                //Skip the first space symbol that follows the delimiter with the source
                                rest.get_first_space(target);

                                //Read the target phrase, it is surrounded by spaces
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                //Check that the weights are good and retrieve them if they are
                                if (process_features<true>(rest, tmp_features_size, tmp_features)) {
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

                                    //Get the language model weights for the target translation
                                    const prob_weight lm_weight = m_lm_query.execute(m_tmp_num_words, m_tmp_word_ids);

                                    LOG_DEBUG << "The phrase: ___" << target << "__ lm-weight: " << lm_weight << END_LOG;

                                    //Initiate a new target entry
                                    source_entry->add_target(target_str, target_uid,
                                            tmp_features_size, tmp_features,
                                            m_tmp_num_words, m_tmp_word_ids, lm_weight);

                                    //Reduce the counter
                                    count_ref--;

                                    LOG_DEBUG1 << "The source/target (" << source_entry->get_source_uid()
                                            << "/" << target_uid << ") counter is " << count_ref << END_LOG;
                                }
                            }

                            /**
                             * Allows to parse the TM model file and do two things depending on the value of the template parameter:
                             * 1. Count the number of valid entries
                             * 2. Build the TM model
                             * NOTE: This two pass parsing is not optimal but we have to do it as we need to know
                             *       the number of valid entries beforehand, an optimization might be needed!
                             * @param count_or_build if true then count if false then build
                             */
                            template<bool count_or_build>
                            inline void parse_tm_file() {

                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source;

                                //Store the source entry
                                tm_source_entry * source_entry = NULL;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;
                                bool is_good_source = false;

                                //Declare an array of weights for temporary use
                                feature_array tmp_features;
                                size_t tmp_features_size = 0;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uid
                                    string next_source_str = source.str();
                                    trim(next_source_str);

                                    LOG_DEBUG << "Got the source phrase: ___" << next_source_str << "___" << END_LOG;

                                    if (source_str != next_source_str) {
                                        size_t size_val = m_sizes[source_uid];
                                        if (count_or_build) {
                                            //Remove the previous entry if the count is zero
                                            if (size_val == 0) {
                                                m_sizes.erase(source_uid);
                                            }
                                        } else {
                                            //Finalize the previous entry if there was one
                                            if (is_good_source && (source_uid != UNDEFINED_PHRASE_ID)) {
                                                LOG_DEBUG1 << "Finishing a source entry for: ___" << source_str
                                                        << "___ uid: " << source_uid << ", count: " << size_val << END_LOG;

                                                m_model.finalize_entry(source_uid);
                                            }
                                        }

                                        //Store the new source string
                                        source_str = next_source_str;

                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);

                                        //Get the current value for the new source id
                                        size_val = m_sizes[source_uid];

                                        LOG_DEBUG1 << "The new source ___" << source_str << "___ id is: "
                                                << source_uid << ", count is: " << size_val << END_LOG;

                                        if (count_or_build) {
                                            //Nothing to be done here when counting entries
                                            LOG_DEBUG1 << "Counting the valid translations for " << source_uid << END_LOG;
                                        } else {
                                            //Check if the new entry is to be entirely skipped
                                            is_good_source = (size_val != 0);

                                            if (is_good_source) {
                                                //Open the new source entry
                                                source_entry = m_model.begin_entry(source_uid, size_val);

                                                LOG_DEBUG1 << "Starting a new source entry for: ___" << source_str
                                                        << "___ uid: " << source_uid << ", count: " << size_val << END_LOG;
                                            } else {
                                                //This source is skipped and if it is the
                                                //last one then we do not need to finalize it
                                                source_uid = UNDEFINED_PHRASE_ID;
                                            }
                                        }
                                    }

                                    //Get the reference to the current size
                                    size_t & size_ref = m_sizes[source_uid];
                                    if (count_or_build) {
                                        //Check that the filter conditions hold
                                        if ((size_ref < m_params.m_trans_limit) &&
                                                is_good_features(line, tmp_features_size, tmp_features)) {
                                            //Increment the count for the given source uid
                                            ++size_ref;

                                            LOG_DEBUG1 << "The new source " << source_uid << " count is " << size_ref << END_LOG;
                                        }
                                    } else {
                                        //If the source entry is not to be skipped, parse 
                                        if (is_good_source && (size_ref > 0)) {
                                            //Parse the rest of the target entry
                                            process_target_entry(source_entry, line, size_ref, tmp_features_size, tmp_features);
                                        } else {
                                            LOG_DEBUG << "Skipping source-target entry: " << line << END_LOG;
                                        }
                                    }

                                    //Update the progress bar status
                                    Logger::update_progress_bar();
                                }

                                if (count_or_build) {
                                    //Check if the last entry resulted in zero score, if yes then remove it
                                    if (m_sizes[source_uid] == 0) {
                                        m_sizes.erase(source_uid);
                                    }
                                } else {
                                    //Finalize the previous entry if there was one
                                    if (source_uid != UNDEFINED_PHRASE_ID) {
                                        m_model.finalize_entry(source_uid);
                                    }

                                    //Finalize the model
                                    m_model.finalize();
                                }
                            }

                            /**
                             * Allows to count and set the number of source phrases
                             */
                            inline void count_source_phrases() {
                                Logger::start_progress_bar(string("Counting phrase translations"));

                                //Count the good entries
                                parse_tm_file<true>();

                                //Set the number of entries into the model
                                m_model.set_num_entries(m_sizes.size());

                                //Re-set the reader to start all over again
                                m_reader.reset();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();

                                LOG_INFO << "The number of valid TM source entries is: " << m_sizes.size() << END_LOG;
                            }

                            /**
                             * Allows to process translations.
                             */
                            inline void process_source_entries() {
                                Logger::start_progress_bar(string("Building translation model"));

                                //Build the model
                                parse_tm_file<false>();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();
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

                        private:
                            //The map storing the model sizes
                            sizes_map m_sizes;
                            //Stores the reference to the model parameters
                            const tm_parameters & m_params;
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
                    }
                }
            }
        }
    }
}

#endif /* TM_BUILDER_HPP */

