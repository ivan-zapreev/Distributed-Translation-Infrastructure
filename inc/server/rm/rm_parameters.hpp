/* 
 * File:   rm_parameters.hpp
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

#ifndef RM_PARAMETERS_HPP
#define RM_PARAMETERS_HPP

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
                namespace rm {

                    /**
                     * This structure stores the reordering model parameters
                     */
                    typedef struct {
                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of reordering model weights
                        size_t m_num_lambdas;

                        //Stores the reordering model weights
                        float m_lambdas[MAX_NUM_RM_FEATURES];

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void verify() {
                            ASSERT_CONDITION_THROW((m_num_lambdas > MAX_NUM_RM_FEATURES),
                                    string("The number of RM features: ") + to_string(m_num_lambdas) +
                                    string(" must be <= ") + to_string(MAX_NUM_RM_FEATURES));
                            ASSERT_CONDITION_THROW(
                                    (m_num_lambdas != SIX_RM_FEATURES) &&
                                    (m_num_lambdas != EIGHT_RM_FEATURES),
                                    string("The number of RM features: ") + to_string(m_num_lambdas) +
                                    string(" must be ") + to_string(SIX_RM_FEATURES) +
                                    string(" or ") + to_string(EIGHT_RM_FEATURES) );
                        }
                    } rm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const rm_parameters & params) {
                        return stream << "RM parameters: [ conn_string = " << params.m_conn_string
                                << ", num_rm_feature_weights = " << params.m_num_lambdas
                                << ", rm_feature_weights = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, RM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << " ]";
                    }
                }
            }
        }
    }
}

#endif /* RM_PARAMETERS_HPP */

