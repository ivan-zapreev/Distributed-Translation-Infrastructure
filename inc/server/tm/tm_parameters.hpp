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

#include "server/tm/tm_configs.hpp"

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

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void verify() {
                            ASSERT_CONDITION_THROW((m_num_lambdas > MAX_NUM_TM_FEATURES),
                                    string("The number of TM features: ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(MAX_NUM_TM_FEATURES));
                            ASSERT_CONDITION_THROW((m_num_lambdas != FOUR_TM_FEATURES),
                                    string("The number of TM features: ") + to_string(m_num_lambdas) +
                                    string(" is not supported, we support only: ") + to_string(FOUR_TM_FEATURES));
                            ASSERT_CONDITION_THROW((m_num_unk_features > MAX_NUM_TM_FEATURES),
                                    string("The number of TM unk features: ") + to_string(m_num_unk_features) +
                                    string(" must be <= ") + to_string(MAX_NUM_TM_FEATURES));
                            ASSERT_CONDITION_THROW((m_num_unk_features != FOUR_TM_FEATURES),
                                    string("The number of TM unk features: ") + to_string(m_num_unk_features) +
                                    string(" is not supported, we support only: ") + to_string(FOUR_TM_FEATURES));
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
                                << ", num_tm_feature_weights = " << params.m_num_lambdas
                                << ", tm_feature_weights = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, TM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << ", translation_limit = " << params.m_trans_limit
                                << ", min_trans_prob = " << params.m_min_tran_prob
                                << " ]";
                    }
                }
            }
        }
    }
}


#endif /* TM_PARAMETERS_HPP */

