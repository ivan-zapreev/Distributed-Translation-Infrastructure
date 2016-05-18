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
                namespace rm {

                    /**
                     * This structure stores the reordering model parameters
                     */
                    struct rm_parameters_struct {
                        //Stores the configuration section name
                        static const string RM_CONFIG_SECTION_NAME;
                        //The RM connection string parameter name
                        static const string RM_CONN_STRING_PARAM_NAME;
                        //The feature weights parameter name
                        static const string RM_WEIGHTS_PARAM_NAME;
                        //Stores the number of lm weight names
                        static constexpr size_t RM_WEIGHT_NAMES_SIZE = 8;
                        //The feature weight names
                        static const string RM_WEIGHT_NAMES[RM_WEIGHT_NAMES_SIZE];

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of reordering model weights
                        size_t m_num_lambdas;

                        //Stores the reordering model weights
                        float m_lambdas[NUM_RM_FEATURES];

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
                                features.push_back(pair<size_t, string>(wcount, RM_WEIGHT_NAMES[idx]));
                                //Increment the feature count
                                ++wcount;
                            }
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            //The number of lambdas must correspond to the expected one
                            ASSERT_CONDITION_THROW((m_num_lambdas != NUM_RM_FEATURES),
                                    string("The number of ") + RM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" must be == ") + to_string(NUM_RM_FEATURES));

                            //The number of lambdas must not exceed the number of enum names
                            ASSERT_CONDITION_THROW((m_num_lambdas > RM_WEIGHT_NAMES_SIZE),
                                    string("The number of ") + RM_WEIGHTS_PARAM_NAME +
                                    string(": ") + to_string(m_num_lambdas) +
                                    string(" exceeds the number of available ") +
                                    string("feature ids: ") + to_string(RM_WEIGHT_NAMES_SIZE));
                        }
                    };

                    //Typedef the structure
                    typedef rm_parameters_struct rm_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const rm_parameters & params) {
                        return stream << "RM parameters: [ conn_string = " << params.m_conn_string
                                << ", " << rm_parameters::RM_WEIGHTS_PARAM_NAME << "[" << params.m_num_lambdas
                                << "] = " << array_to_string<float>(params.m_num_lambdas,
                                params.m_lambdas, RM_FEATURE_WEIGHTS_DELIMITER_STR)
                                << " ]";
                    }
                }
            }
        }
    }
}

#endif /* RM_PARAMETERS_HPP */

