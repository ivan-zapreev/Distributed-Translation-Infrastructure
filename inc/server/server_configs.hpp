/* 
 * File:   server_configs.hpp
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
 * Created on February 23, 2016, 4:52 PM
 */

#include "server_consts.hpp"

#ifndef SERVER_CONFIGS_HPP
#define SERVER_CONFIGS_HPP

#include <string>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                //Stores the zero log_e probability for the partial score or future cost
                static constexpr prob_weight UNKNOWN_LOG_PROB_WEIGHT = -2300.0f; //This is the log_e (ln) weight
                //The zero like value for log_e probability weight
                static constexpr prob_weight ZERO_LOG_PROB_WEIGHT = -230.0f; //This is the log_e (ln) weight

                //This macro defines whether or not the software is to be compiled
                //in the tuning mode or not. The tuning mode allows to dump the
                //search lattices and related data but it is also slower.
#define IS_SERVER_TUNING_MODE false

                namespace decoder {
                }

                namespace tm {
                    //Stores the maximum number of the translation model features
                    static constexpr size_t MAX_NUM_TM_FEATURES = 12u;

                    //The considered maximum length of the target phrase
                    static constexpr uint16_t TM_MAX_TARGET_PHRASE_LEN = 7u;
                }

                namespace lm {
                    //Stores the number of the language model features
                    static constexpr size_t MAX_NUM_LM_FEATURES = 1u;

                    //The considered maximum length of the N-gram 
                    static constexpr uint16_t LM_M_GRAM_LEVEL_MAX = 5u;
                    //The maximum considered length of the m-gram history
                    static constexpr uint16_t LM_HISTORY_LEN_MAX = LM_M_GRAM_LEVEL_MAX - 1;
                    //The considered maximum length of the N-gram query
                    static constexpr uint16_t LM_MAX_QUERY_LEN = tm::TM_MAX_TARGET_PHRASE_LEN + LM_HISTORY_LEN_MAX;
                }

                namespace rm {
                    //Stores the number of the reordering model features
                    static constexpr int8_t MAX_NUM_RM_FEATURES = EIGHT_RM_FEATURES;
                    //Stores the half of the maximum number of the reordering model features
                    static constexpr int8_t HALF_MAX_NUM_RM_FEATURES = MAX_NUM_RM_FEATURES / 2;
                }
            }
        }
    }
}

//INclude server constants as they depend on server configs
#include "server/server_consts.hpp"

#endif /* SERVER_CONFIGS_HPP */

