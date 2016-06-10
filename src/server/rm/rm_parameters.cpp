/* 
 * File:   rm_parameters.cpp
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
 * Created on May 17, 2016, 12:24 AM
 */

#include "server/rm/rm_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    const string rm_parameters_struct::RM_CONFIG_SECTION_NAME = "Reordering Models";
                    const string rm_parameters_struct::RM_CONN_STRING_PARAM_NAME = "rm_conn_string";
                    const string rm_parameters_struct::RM_WEIGHTS_PARAM_NAME = "rm_feature_weights";
                    const string rm_parameters_struct::RM_WEIGHT_NAMES[MAX_NUM_RM_FEATURES] = {
                        RM_WEIGHTS_PARAM_NAME + string("[0]"),
                        RM_WEIGHTS_PARAM_NAME + string("[1]"),
                        RM_WEIGHTS_PARAM_NAME + string("[2]"),
                        RM_WEIGHTS_PARAM_NAME + string("[3]"),
                        RM_WEIGHTS_PARAM_NAME + string("[4]"),
                        RM_WEIGHTS_PARAM_NAME + string("[5]"),
                        RM_WEIGHTS_PARAM_NAME + string("[6]"),
                        RM_WEIGHTS_PARAM_NAME + string("[7]")
                    };
                    size_t rm_parameters_struct::RM_WEIGHT_GLOBAL_IDS[MAX_NUM_RM_FEATURES] = {};
                }
            }
        }
    }
}