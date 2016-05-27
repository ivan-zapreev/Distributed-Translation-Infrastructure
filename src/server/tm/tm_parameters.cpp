/* 
 * File:   tm_parameters.cpp
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
 * Created on May 17, 2016, 12:25 AM
 */

#include "server/tm/tm_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    const string tm_parameters_struct::TM_CONFIG_SECTION_NAME = "Translation Models";
                    const string tm_parameters_struct::TM_CONN_STRING_PARAM_NAME = "tm_conn_string";
                    const string tm_parameters_struct::TM_WEIGHTS_PARAM_NAME = "tm_feature_weights";
                    const string tm_parameters_struct::TM_UNK_FEATURE_PARAM_NAME = "tm_unk_features";
                    const string tm_parameters_struct::TM_TRANS_LIM_PARAM_NAME = "tm_trans_lim";
                    const string tm_parameters_struct::TM_MIN_TRANS_PROB_PARAM_NAME = "tm_min_trans_prob";
                    const string tm_parameters_struct::TM_WEIGHT_NAMES[NUM_TM_FEATURES] = {
                        TM_WEIGHTS_PARAM_NAME + string("[0]"),
                        TM_WEIGHTS_PARAM_NAME + string("[1]"),
                        TM_WEIGHTS_PARAM_NAME + string("[2]"),
                        TM_WEIGHTS_PARAM_NAME + string("[3]"),
                        TM_WEIGHTS_PARAM_NAME + string("[4]")
                    };
                }
            }
        }
    }
}