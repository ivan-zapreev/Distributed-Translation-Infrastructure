/* 
 * File:   tm_configs.hpp
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
 * Created on February 9, 2016, 11:53 AM
 */

#ifndef TM_CONFIGS_HPP
#define TM_CONFIGS_HPP

#include <inttypes.h>
#include <string>

#include "server/common/models/phrase_uid.hpp"

using namespace std;

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        namespace __unk_phrase {
                            //Stores the unknown source phrase string, should be configurable
                            static const string TM_UNKNOWN_SOURCE_STR = string("UNK");
                            //Stores the unknown target phrase string, should be configurable
                            static const string TM_UNKNOWN_TARGET_STR = string("<unk>");

                            //The default inverse phrase translation probability φ(s|t)
                            const float UNK_SCT_LOG_PROB_WEIGHT = -10.0;
                            //The Stores the default inverse lexical weighting lex(s|t)
                            const float UNK_LSCT_LOG_PROB_WEIGHT = -10.0;
                            //The Stores the default direct phrase translation probability φ(t | s)
                            const float UNK_TCS_LOG_PROB_WEIGHT = -10.0;
                            //The Stores the default direct lexical weighting lex(t|s)
                            const float UNK_LTCS_LOG_PROB_WEIGHT = -10.0;
                        }

                        namespace __tm_basic_model {
                            //Influences the number of buckets that will be created for the basic model implementations
                            static constexpr double SOURCES_BUCKETS_FACTOR = 3.0;
                        }
                    }
                }
            }
        }
    }
}

#endif /* TM_CONFIGS_HPP */

