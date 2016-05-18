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
                    struct rm_parameters_struct {
                        //Stores the configuration section name
                        static const string RM_CONFIG_SECTION_NAME;
                        //The RM connection string parameter name
                        static const string RM_CONN_STRING_PARAM_NAME;
                        //The feature weights parameter name
                        static const string RM_WEIGHTS_PARAM_NAME;

                        //The the connection string needed to connect to the model
                        string m_conn_string;

                        //Stores the number of reordering model weights
                        size_t m_num_lambdas;

                        //Stores the reordering model weights
                        float m_lambdas[NUM_RM_FEATURES];
                        
                        //Store the feature id offset for globally storing feature values
                        //Is only set to a valid value when the lattice generation is on.
                        size_t m_w_id_offset;

                        /**
                         * Store the feature ids in a form of an enumeration
                         */
                        enum rm_weight_ids {
                            RM_WEIGHTS_PARAM_ID_0 = 0,
                            RM_WEIGHTS_PARAM_ID_1 = RM_WEIGHTS_PARAM_ID_0 + 1,
                            RM_WEIGHTS_PARAM_ID_2 = RM_WEIGHTS_PARAM_ID_1 + 1,
                            RM_WEIGHTS_PARAM_ID_3 = RM_WEIGHTS_PARAM_ID_2 + 1,
                            RM_WEIGHTS_PARAM_ID_4 = RM_WEIGHTS_PARAM_ID_3 + 1,
                            RM_WEIGHTS_PARAM_ID_5 = RM_WEIGHTS_PARAM_ID_4 + 1,
                            RM_WEIGHTS_PARAM_ID_6 = RM_WEIGHTS_PARAM_ID_5 + 1,
                            RM_WEIGHTS_PARAM_ID_7 = RM_WEIGHTS_PARAM_ID_6 + 1,
                            rm_weight_ids_size = RM_WEIGHTS_PARAM_ID_7 + 1
                        };

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param wconsumer [out] a unique feature weights consumer name,
                         *                        its uniqueness is checked in the caller
                         * @param wcount [in/out] the number of feature weights up until
                         *                        now, when called, when the call if finished
                         *                        the number of feature weights including the
                         *                        added ones.
                         * @param features [out] the vector the features will be appended to
                         */
                       void add_weight_names(string & wconsumer, size_t & wcount, vector<string> & features) {
                            //Set the id offset
                            m_w_id_offset = wcount;
                            
                            //Add the feature weight names and increment the weight count
                            for (size_t idx = 0; idx < m_num_lambdas; ++idx) {
                                features.push_back(RM_WEIGHTS_PARAM_NAME +
                                        string("[") + to_string(idx) + string("]"));
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

