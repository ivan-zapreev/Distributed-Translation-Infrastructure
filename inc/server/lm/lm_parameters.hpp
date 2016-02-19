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

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/string_utils.hpp"

#include "server/lm/lm_configs.hpp"

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
                    typedef struct {
                        //The the connection string needed to connect to the model
                        string m_conn_string;


                        //Stores the number of language model weights
                        size_t m_num_lambdas;

                        //Stores the language model weights
                        float m_lambdas[MAX_NUM_LM_FEATURES];
                        
                        /**
                         * Allows to detect that the lm weight is set and needs to be used
                         * @return the lm weight
                         */
                        bool is_lm_weight() const {
                            return (m_num_lambdas >=1) && (m_lambdas[0] != 1.0) && (m_lambdas[0] != 0.0);
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
                        void verify() {
                            ASSERT_CONDITION_THROW((m_num_lambdas > MAX_NUM_LM_FEATURES),
                                    string("The number of LM features: ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(MAX_NUM_LM_FEATURES));
                        }
                    } lm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const lm_parameters & params) {
                        return stream << "LM parameters: [ conn_string = " << params.m_conn_string
                                << ", num_lm_feature_weights = " << params.m_num_lambdas
                                << ", lm_feature_weights = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, LM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << " ]";
                    }
                }
            }
        }
    }
}

#endif /* LANGUAGE_MODEL_PARAMETERS_HPP */

