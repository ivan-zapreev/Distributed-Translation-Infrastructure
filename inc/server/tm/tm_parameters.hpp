/* 
 * File:   tm_parameters.hpp
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
 * Created on February 4, 2016, 11:53 AM
 */

#ifndef TM_PARAMETERS_HPP
#define TM_PARAMETERS_HPP

#include <string>
#include <ostream>
#include <utility>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/text/string_utils.hpp"

#include "server/server_configs.hpp"
#include "server/common/feature_id_registry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {

                    /**
                     * This structure stores the translation model parameters
                     */
                    struct tm_parameters_struct {
                        //Stores the configuration section name
                        static const string TM_CONFIG_SECTION_NAME;
                        //The TM connection string parameter name
                        static const string TM_CONN_STRING_PARAM_NAME;
                        //The feature weights parameter name
                        static const string TM_WEIGHTS_PARAM_NAME;
                        //The unknown translation feature weights parameter name
                        static const string TM_UNK_FEATURE_PARAM_NAME;
                        //The translation limit parameter name
                        static const string TM_TRANS_LIM_PARAM_NAME;
                        //The minimum translation probability parameter name
                        static const string TM_MIN_TRANS_PROB_PARAM_NAME;
                        //The feature weight names
                        static const string TM_WEIGHT_NAMES[MAX_NUM_TM_FEATURES];
                        //The feature weight names
                        static size_t TM_WEIGHT_GLOBAL_IDS[MAX_NUM_TM_FEATURES];
                        //The word penalty weight parameter name
                        static const string TM_WP_LAMBDA_PARAM_NAME;
                        //The global id of the word penalty feature
                        static size_t TM_WP_LAMBDA_GLOBAL_ID;
                        //The index of the word penalty parameter lambda
                        static size_t TM_PHRASE_PENALTY_LAMBDA_IDX;

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of translation model weights
                        size_t m_num_lambdas;

                        //Stores the translation model weights
                        float m_lambdas[MAX_NUM_TM_FEATURES];

                        //Stores the number of unk entry features
                        size_t m_num_unk_features;

                        //Stores the unk entry features
                        float m_unk_features[MAX_NUM_TM_FEATURES];

                        //Stores the translation limit - the number of top translation
                        //to be read from the translation model file per source phrase
                        size_t m_trans_limit;

                        //The minimum translation probability limit - defines the
                        //translation entries that are to be ignored when reading
                        //model, this is not log probability, and also is used for
                        //without feature weights
                        float m_min_tran_prob;
                        
                        //Stores the word penalty lambda - the cost of each target word
                        float m_wp_lambda;

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param registry the feature registry entity
                         */
                        inline void get_weight_names(feature_id_registry & registry) {
                            //Declare the feature source 
                            const string source = __FILENAME__;
                            //Add the feature weight names and increment the weight count
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                //Add the feature global id to its name mapping
                                registry.add_feature(source, TM_WEIGHT_NAMES[idx], TM_WEIGHT_GLOBAL_IDS[idx]);
                            }
                            //Add the word penalty feature
                            registry.add_feature(source, TM_WP_LAMBDA_PARAM_NAME, TM_WP_LAMBDA_GLOBAL_ID);
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas > MAX_NUM_TM_FEATURES),
                                    string("The number of ") + TM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(MAX_NUM_TM_FEATURES));

                            //The number of features must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_unk_features > MAX_NUM_TM_FEATURES),
                                    string("The number of ") + TM_UNK_FEATURE_PARAM_NAME +
                                    string(": ") + to_string(m_num_unk_features) +
                                    string(" must be <= ") + to_string(MAX_NUM_TM_FEATURES));

                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas !=  m_num_unk_features),
                                    string("The number of ") + TM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" != ") + TM_UNK_FEATURE_PARAM_NAME +
                                    string(":") + to_string(m_num_unk_features));

                            //Check that the UNK features are not zero - this will result in a -inf log_e weights!
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                ASSERT_CONDITION_THROW((m_unk_features[idx] == 0.0),
                                        string("Translation parameters ") +
                                        TM_UNK_FEATURE_PARAM_NAME + string("[") +
                                        to_string(idx) + string("] is zero!"));
                            }
                        }
                    };

                    //Typedef the structure
                    typedef tm_parameters_struct tm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const tm_parameters & params) {
                        return stream << "TM parameters: [ conn_string = " << params.m_conn_string
                                << ", " << tm_parameters::TM_WEIGHTS_PARAM_NAME << "[" << params.m_num_lambdas
                                << "] = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, TM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << ", " << tm_parameters::TM_UNK_FEATURE_PARAM_NAME << "[" << params.m_num_unk_features
                                << "] = " << array_to_string<float>(params.m_num_unk_features,
                                params.m_unk_features, TM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << ", " << tm_parameters::TM_TRANS_LIM_PARAM_NAME << " = " << params.m_trans_limit
                                << ", " << tm_parameters::TM_MIN_TRANS_PROB_PARAM_NAME << " = " << params.m_min_tran_prob
                                << ", " << tm_parameters::TM_WP_LAMBDA_PARAM_NAME << " = " << params.m_wp_lambda
                                << " ]";
                    }
                }
            }
        }
    }
}


#endif /* TM_PARAMETERS_HPP */

