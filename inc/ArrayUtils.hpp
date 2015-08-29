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

#include <string>       // std::stringstream
#include <cstdlib>      // std::qsort std::bsearch
#include <algorithm>    // std::sort

#include "Logger.hpp"
#include "Globals.hpp"
#include "Exceptions.hpp"

using namespace std;
using namespace uva::smt::exceptions;

using uva::smt::logging::Logger;

namespace uva {
    namespace smt {
        namespace utils {
            namespace array {

                /**
                 * This is a search algorithm for some ordered array, here we use bsearch from <cstdlib>
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param mid_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE, typename INDEX_TYPE, typename KEY_TYPE >
                bool bsearch(const ARR_ELEM_TYPE * array, INDEX_TYPE l_idx, INDEX_TYPE u_idx, const KEY_TYPE key, INDEX_TYPE & mid_pos) {
                    if (DO_SANITY_CHECKS && ((l_idx < 0) || (l_idx > u_idx))) {
                        stringstream msg;
                        msg << "Impossible binary search parameters, l_idx = "
                                << SSTR(l_idx) << ", u_idx = "
                                << SSTR(u_idx) << "!";
                        throw Exception(msg.str());
                    }

                    //Do the binary search
                    const ARR_ELEM_TYPE * arr_ptr = array + l_idx;
                    const size_t size = (u_idx - l_idx) + 1;

                    LOG_DEBUG4 << "Doing binary search in array: " << SSTR(arr_ptr)
                            << ", size: " << SSTR(size) << END_LOG;

                    const ARR_ELEM_TYPE * result = static_cast<const ARR_ELEM_TYPE*> (std::bsearch(&key, arr_ptr,
                            size, sizeof (ARR_ELEM_TYPE), [] (const void * p_a, const void * p_b) -> int {
                                const KEY_TYPE & v_a = (const KEY_TYPE) *(static_cast<const ARR_ELEM_TYPE*> (p_a));
                                const KEY_TYPE & v_b = (const KEY_TYPE) *(static_cast<const ARR_ELEM_TYPE*> (p_b));
                                        LOG_DEBUG4 << "Comparing: v_a = " << SSTR(v_a) << ", and v_b = " << SSTR(v_b) << END_LOG;
                                if (v_a < v_b) {
                                    return -1;
                                } else {
                                    if (v_a > v_b) {
                                        return +1;
                                    } else {
                                        return 0;
                                    }
                                }
                            }));

                    //The resulting index is the difference between
                    //pointers, if the elements was found
                    if (result != NULL) {
                        //The found position is with respect to the beginning of the entire array
                        mid_pos = (INDEX_TYPE) (result - array);
                        LOG_DEBUG4 << "The element " << key << " is found, array[ "
                                << SSTR(mid_pos) << " ] = " << SSTR((KEY_TYPE) array[mid_pos]) << END_LOG;
                        return true;
                    } else {
                        LOG_DEBUG4 << "The element is NOT found" << END_LOG;
                        return false;
                    }
                }

                /**
                 * This methos is used to do <algorithm> std::sort on an array
                 * of structures convertable to some simple comparable type.
                 * This method does the progress bar update, if needed
                 * @param ELEM_TYPE the array element type
                 * @param BASE_TYPE the base type the array element type has to
                 * be custable to, the base type must have implemented "operator <"
                 * @param IS_PROGRESS if true the progress bar will be updated,
                 * otherwise not, default is true
                 * @param array_begin the pointer to the array's first element
                 * @param array_size the size of the array
                 */
                template<typename ELEM_TYPE, typename BASE_TYPE, bool IS_PROGRESS = true >
                void sort(ELEM_TYPE * array_begin, const TShortId array_size) {
                    //Do not do sorting if the array size is less than two
                    if (array_size > 1) {
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        std::sort(array_begin, array_begin + array_size,
                                [] (const ELEM_TYPE & first, const ELEM_TYPE & second) -> bool {
                                    if (IS_PROGRESS) {
                                        //Update the progress bar status
                                        Logger::updateProgressBar();
                                    }
                                    //Return the result
                                    return (((BASE_TYPE) first) < ((BASE_TYPE) second));
                                });
                    }
                }

                /**
                 * This methos is used to do <cstdlib> std::qsort on an array of
                 * structures convertable to some simple comparable type.
                 * This method does the progress bar update, if needed. Note that,
                 * this algorithm assumes that there are no duplicate elements
                 * in the array. I.e. when two elements are compared (as the given
                 * base type) one is always smaller than another! Thus, if used
                 * on arrays with duplicates, can be slower than usual due to
                 * extra element moving.
                 * @param ELEM_TYPE the array element type
                 * @param BASE_TYPE the base type the array element type has to
                 * be custable to, the base type must have implemented "operator <"
                 * @param IS_PROGRESS if true the progress bar will be updated,
                 * otherwise not, default is true
                 * @param array the pointer to the array's first element
                 * @param size the number of array elements
                 */
                template<typename ELEM_TYPE, typename BASE_TYPE, bool IS_PROGRESS = true >
                void qsort(ELEM_TYPE * array, const size_t size) {
                    //Order the N-gram array as it is not most likely unordered!
                    std::qsort(array, size, sizeof (ELEM_TYPE),
                            [] (const void* first, const void* second) -> int {
                                if (IS_PROGRESS) {
                                    //Update the progress bar status
                                    Logger::updateProgressBar();
                                }
                                //NOTE: Since the array contains unique pairs there is no situation when they are equal! So we never return 0!
                                return (((BASE_TYPE) (*(ELEM_TYPE*) first)) < ((BASE_TYPE) (*(ELEM_TYPE*) second))) ? -1 : +1;
                            });
                }
            }
        }
    }
}

#endif	/* ARRAYUTILS_HPP */

