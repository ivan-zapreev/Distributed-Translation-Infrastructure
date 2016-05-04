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
                            : m_params(params), m_model(model), m_reader(reader),
                            m_lm_query(lm_configurator::allocate_fast_query_proxy()),
                            m_tmp_num_words(0) {

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
                                //ToDo: Load the model data into memory and filter

                                //ToDo: Convert the loaded data into the model

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
                    };
                }
            }
        }
    }
}

#endif /* TM_LIMITING_BUILDER_HPP */

