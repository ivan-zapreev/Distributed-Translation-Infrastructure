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

#include <cstdlib>      // std::qsort
#include <algorithm>    // std::sort

#include "Logger.hpp"
#include "Globals.hpp"

using namespace std;
using uva::smt::logging::Logger;

namespace uva {
    namespace smt {
        namespace utils {
            namespace array {

                /**
                 * This is a binary search algorithm for some ordered array
                 * @param array the pointer to the first array element
                 * @param lowerbound the initial left border index for searching
                 * @param upperbound the initial right border index for searching
                 * @param key the key we are searching for
                 * @param position the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 */
                template<typename ARR_ELEM_TYPE, typename INDEX_TYPE, typename KEY_TYPE >
                bool binarySearch(const ARR_ELEM_TYPE * array, INDEX_TYPE lowerbound, INDEX_TYPE upperbound, const KEY_TYPE key, INDEX_TYPE & position) {
                    // To start, find the subscript of the middle position.
                    position = (lowerbound + upperbound) / 2;

                    KEY_TYPE found_key = static_cast<KEY_TYPE> (array[position]);

                    LOG_DEBUG3 << "Starting the binary search [l,u] = [" << lowerbound
                            << ", " << upperbound << "], searched key: " << key
                            << ", first considered pos: " << position
                            << ", key: " << found_key << END_LOG;

                    //Perform the search
                    while ((found_key != key) && (lowerbound <= upperbound)) {

                        //Decide on which way to go
                        if (found_key > key) // If the number is > key, ..
                        { // decrease position by one.
                            upperbound = position - 1;
                        } else { // Else, increase position by one.
                            lowerbound = position + 1;
                        }

                        //Get the next element
                        position = (lowerbound + upperbound) / 2;
                        found_key = static_cast<KEY_TYPE> (array[position]);

                        LOG_DEBUG3 << "Next considered [l,u] = [" << lowerbound
                                << ", " << upperbound << "], pos: " << position
                                << ", key: " << found_key << END_LOG;
                    }

                    //Return true if the element was found
                    return (lowerbound <= upperbound);
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
                 * @param array_end the pointer to the array's last element
                 */
                template<typename ELEM_TYPE, typename BASE_TYPE, bool IS_PROGRESS = true >
                void sort(ELEM_TYPE * array_begin, ELEM_TYPE * array_end) {
                    //Order the N-gram array as it is unordered and we will binary search it later!
                    std::sort(array_begin, array_end,
                            [] (const ELEM_TYPE & first, const ELEM_TYPE & second) -> bool {
                                if (IS_PROGRESS) {
                                    //Update the progress bar status
                                    Logger::updateProgressBar();
                                }
                                //Return the result
                                return (((BASE_TYPE) first) < ((BASE_TYPE) second));
                            });
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

