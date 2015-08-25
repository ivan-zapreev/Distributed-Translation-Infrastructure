/* 
 * File:   ArrayUtils.hpp
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
 * Created on August 25, 2015, 2:54 PM
 */

#ifndef ARRAYUTILS_HPP
#define	ARRAYUTILS_HPP

#include "Logger.hpp"
#include "Globals.hpp"

using uva::smt::logging::Logger;

namespace uva {
    namespace smt {
        namespace utils {
            namespace array {

                template<typename ARR_ELEM_TYPE, typename INDEX_TYPE, typename KEY_TYPE >
                bool binarySearch(ARR_ELEM_TYPE * array, INDEX_TYPE lowerbound, INDEX_TYPE upperbound, KEY_TYPE key, INDEX_TYPE & position) {
                    // To start, find the subscript of the middle position.
                    position = (lowerbound + upperbound) / 2;

                    while ((static_cast<KEY_TYPE> (array[position]) != key) && (lowerbound <= upperbound)) {
                        if (static_cast<KEY_TYPE> (array[position]) > key) // If the number is > key, ..
                        { // decrease position by one.
                            upperbound = position - 1;
                        } else { // Else, increase position by one.
                            lowerbound = position + 1;
                        }
                        position = (lowerbound + upperbound) / 2;
                    }

                    //Return true if the element was found
                    return (lowerbound <= upperbound);
                }
            }
        }
    }
}

#endif	/* ARRAYUTILS_HPP */

