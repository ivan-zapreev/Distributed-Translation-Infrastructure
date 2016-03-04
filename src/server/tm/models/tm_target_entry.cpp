/* 
 * File:   tm_target_entry.cpp
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
 * Created on March 02, 2016, 16:21 AM
 */

#include "server/tm/models/tm_target_entry.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        //Initialize the unknown target entry UID constant 
                        template<uint8_t num_features>
                        const phrase_uid tm_target_entry_temp<num_features>::UNKNOWN_TARGET_ENTRY_UID = combine_phrase_uids(UNKNOWN_PHRASE_ID, UNKNOWN_PHRASE_ID);
                        
                        //Instantiate the template
                        template class tm_target_entry_temp<NUM_TM_FEATURES>;
                    }
                }
            }
        }
    }
}