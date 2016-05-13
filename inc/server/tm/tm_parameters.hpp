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
                    typedef struct {
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
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas != NUM_TM_FEATURES),
                                    string("The number of ") + TM_FEATURE_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be == ") + to_string(NUM_TM_FEATURES));

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
                    } tm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const tm_parameters & params) {
                        return stream << "TM parameters: [ conn_string = " << params.m_conn_string
                                << ", " << TM_FEATURE_PARAM_NAME << "[" << params.m_num_lambdas
                                << "] = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, TM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << ", " << TM_TRANS_LIM_PARAM_NAME << " = " << params.m_trans_limit
                                << ", " << TM_MIN_TRANS_PROB_PARAM_NAME << " = " << params.m_min_tran_prob
                                << " ]";
                    }
                }
            }
        }
    }
}


#endif /* TM_PARAMETERS_HPP */

