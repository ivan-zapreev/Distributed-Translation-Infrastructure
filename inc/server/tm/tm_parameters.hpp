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
#include "common/utils/string_utils.hpp"

#include "server/server_configs.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::text;

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
                        //Stores the number of lm weight names
                        static constexpr size_t TM_WEIGHT_NAMES_SIZE = 6;
                        //The feature weight names
                        static const string TM_WEIGHT_NAMES[TM_WEIGHT_NAMES_SIZE];

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of translation model weights
                        size_t m_num_lambdas;

                        //Stores the translation model weights
                        float m_lambdas[NUM_TM_FEATURES];

                        //Stores the number of unk entry features
                        size_t m_num_unk_features;

                        //Stores the unk entry features
                        float m_unk_features[NUM_TM_FEATURES];

                        //Stores the translation limit - the number of top translation
                        //to be read from the translation model file per source phrase
                        size_t m_trans_limit;

                        //The minimum translation probability limit - defines the
                        //translation entries that are to be ignored when reading
                        //model, this is not log probability, and also is used for
                        //without feature weights
                        float m_min_tran_prob;

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param wcount [in/out] the number of feature weights up until
                         *                        now, when called, when the call if finished
                         *                        the number of feature weights including the
                         *                        added ones.
                         * @param features [out] the vector the features will be appended to
                         */
                        void add_weight_names(size_t & wcount, vector<pair<size_t, string>> &features) {
                            //Add the feature weight names and increment the weight count
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                //Add the feature global id to its name mapping
                                features.push_back(pair<size_t, string>(wcount, TM_WEIGHT_NAMES[idx]));
                                //Increment the feature count
                                ++wcount;
                            }
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas != NUM_TM_FEATURES),
                                    string("The number of ") + TM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be == ") + to_string(NUM_TM_FEATURES));

                            //The number of lambdas must not exceed the number of enum names
                            ASSERT_CONDITION_THROW((m_num_lambdas > TM_WEIGHT_NAMES_SIZE),
                                    string("The number of ") + TM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" exceeds the number of available ") +
                                    string("feature ids: ") + to_string(TM_WEIGHT_NAMES_SIZE));

                            //The number of features must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_unk_features != NUM_TM_FEATURES),
                                    string("The number of ") + TM_UNK_FEATURE_PARAM_NAME +
                                    string(": ") + to_string(m_num_unk_features) +
                                    string(" must be == ") + to_string(NUM_TM_FEATURES));

                            //Check that the UNK features are not zero - this will result in a -inf log10 weights!
                            for (size_t idx = 0; idx < NUM_TM_FEATURES; ++idx) {
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
                                << " ]";
                    }
                }
            }
        }
    }
}


#endif /* TM_PARAMETERS_HPP */

