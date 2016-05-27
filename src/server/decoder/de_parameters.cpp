/* 
 * File:   dec_parameters.cpp
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
 * Created on May 17, 2016, 12:21 AM
 */

#include "server/decoder/de_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    const string de_parameters_struct::DE_CONFIG_SECTION_NAME = "Decoding Options";
                    const string de_parameters_struct::DE_PRUNING_THRESHOLD_PARAM_NAME = "de_pruning_threshold";
                    const string de_parameters_struct::DE_STACK_CAPACITY_PARAM_NAME = "de_stack_capacity";
                    const string de_parameters_struct::DE_MAX_SP_LEN_PARAM_NAME = "de_max_source_phrase_length";
                    const string de_parameters_struct::DE_MAX_TP_LEN_PARAM_NAME = "de_max_target_phrase_length";
                    const string de_parameters_struct::DE_WORD_PENALTY_PARAM_NAME = "de_word_penalty";
                    const string de_parameters_struct::DE_DIST_LIMIT_PARAM_NAME = "de_dist_lim";
                    const string de_parameters_struct::DE_LD_PENALTY_PARAM_NAME = "de_lin_dist_penalty";
                    const string de_parameters_struct::DE_IS_GEN_LATTICE_PARAM_NAME = "de_is_gen_lattice";
                    const string de_parameters_struct::DE_LATTICES_FOLDER_PARAM_NAME = "de_lattices_folder";
                    const string de_parameters_struct::DE_LI2N_FILE_EXT_PARAM_NAME = "de_lattice_id2name_file_ext";
                    const string de_parameters_struct::DE_SCORES_FILE_EXT_PARAM_NAME = "de_feature_scores_file_ext";
                    const string de_parameters_struct::DE_LATTICE_FILE_EXT_PARAM_NAME = "de_lattice_file_ext";
                }
            }
        }
    }
}