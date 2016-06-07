/*
 * File:   rm_entry.cpp
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
 * Created on June 7, 2016, 14:05 AM
 */

#include "server/rm/models/rm_entry.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace models {
                        //Default initialize with zero and negative values
                        int8_t rm_entry::NUMBER_OF_FEATURES = 0;
                        int8_t rm_entry::FROM_POSITIONS[HALF_MAX_NUM_RM_FEATURES] = {};
                        int8_t rm_entry::TO_POSITIONS[HALF_MAX_NUM_RM_FEATURES] = {};

                        ostream & operator<<(ostream & stream, const rm_entry & entry) {
                            return stream << "[ uid: " << entry.m_uid << ", weights: "
                                    << array_to_string<prob_weight>(rm_entry::NUMBER_OF_FEATURES, entry.m_weights) << " ]";
                        }
                    }
                }
            }
        }
    }
}