/* 
 * File:   language_model_parameters.hpp
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
 * Created on February 4, 2016, 11:52 AM
 */

#ifndef LANGUAGE_MODEL_PARAMETERS_HPP
#define LANGUAGE_MODEL_PARAMETERS_HPP

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
                namespace lm {

                    /**
                     * This structure is needed to store the language model parameters
                     */
                    struct lm_parameters_struct {
                        //Stores the configuration section name
                        static const string LM_CONFIG_SECTION_NAME;
                        //The LM connection string parameter name
                        static const string LM_CONN_STRING_PARAM_NAME;
                        //The feature weights parameter name
                        static const string LM_WEIGHTS_PARAM_NAME;
                        //The feature weight names
                        static const string LM_WEIGHT_NAMES[NUM_LM_FEATURES];

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of language model weights
                        size_t m_num_lambdas;

                        //Stores the language model weights
                        float m_lambdas[NUM_LM_FEATURES];

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param wcount [in/out] the number of feature weights up until
                         *                        now, when called, when the call if finished
                         *                        the number of feature weights including the
                         *                        added ones.
                         * @param features [out] the vector the features will be appended to
                         */
                        void get_weight_names(size_t & wcount, vector<pair<size_t, string>> &features) {
                            //Add the feature weight names and increment the weight count
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                //Add the feature global id to its name mapping
                                features.push_back(pair<size_t, string>(wcount, LM_WEIGHT_NAMES[idx]));
                                //Increment the feature count
                                ++wcount;
                            }
                        }

                        /**
                         * Allows to detect that the lm weight is set and needs to be used
                         * @return true if we need to multiply with the lambda weight otherwise not.
                         */
                        bool is_lm_weight() const {
                            return (m_num_lambdas >= 1) && (m_lambdas[0] != 1.0) && (m_lambdas[0] != 0.0);
                        }

                        /**
                         * Allows to retrieve the language model m-gram weight
                         * @return the language model m-gram weight
                         */
                        inline const float & get_lm_weight() const {
                            return m_lambdas[0];
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas != NUM_LM_FEATURES),
                                    string("The number of ") + LM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be == ") + to_string(NUM_LM_FEATURES));

                            //The Language model weight must not be zero or negative
                            ASSERT_CONDITION_THROW((m_lambdas[0] <= 0.0),
                                    string("Improper value of ") + LM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_lambdas[0]) + string(" must be > 0.0 "));

                        }
                    };

                    //Typedef the structure
                    typedef lm_parameters_struct lm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const lm_parameters & params) {
                        return stream << "LM parameters: [ conn_string = " << params.m_conn_string
                                << ", " << lm_parameters::LM_WEIGHTS_PARAM_NAME << "[" << params.m_num_lambdas
                                << "] = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, LM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << " ]";
                    }
                }
            }
        }
    }
}

#endif /* LANGUAGE_MODEL_PARAMETERS_HPP */

