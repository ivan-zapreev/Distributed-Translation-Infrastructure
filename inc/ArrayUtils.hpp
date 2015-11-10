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
#include <algorithm>    // std::sort std::abs
#include <functional>   // std::function

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
                 * Define the function type for the comparison function
                 */
                template<typename ELEM_TYPE>
                struct T_IS_EXT_COMPARE_FUNC {
                    typedef std::function<int(const ELEM_TYPE &, const ELEM_TYPE &) > func_type;
                    typedef int(* func_ptr)(const ELEM_TYPE &, const ELEM_TYPE &);
                };

                /**
                 * Define the function type for the comparison function
                 */
                template<typename ELEM_TYPE>
                struct T_IS_COMPARE_FUNC {
                    typedef std::function<bool(const ELEM_TYPE &, const ELEM_TYPE &) > func_type;
                    typedef bool(* func_ptr)(const ELEM_TYPE &, const ELEM_TYPE &);
                };

                //NOTE: Do the binary search, note that we do not take care of index
                //underflows as they are signed. Yet, we might want to take into
                //account the overflows, although these are also not that threatening
                //the reason is that the actual array index is TShortId and we use
                //for index iterations a much longer but signed data type TLongId
#define BSEARCH_ONE_FIELD(FIELD_NAME)                                                               \
                if (DO_SANITY_CHECKS && ((l_idx < 0) || (l_idx > u_idx))) {                         \
                        stringstream msg;                                                           \
                        msg << "Impossible binary search parameters, l_idx = "                      \
                                << SSTR(l_idx) << ", u_idx = "                                      \
                                << SSTR(u_idx) << "!";                                              \
                        throw Exception(msg.str());                                                 \
                    } else {                                                                        \
                        TSLongId mid_pos;                                                           \
                        while (l_idx <= u_idx) {                                                    \
                            mid_pos = (l_idx + u_idx) / 2;                                          \
                            LOG_DEBUG4 << "l_idx = " << SSTR(l_idx) << ", u_idx = "                 \
                                    << SSTR(u_idx) << ", mid_pos = " << SSTR(mid_pos) << END_LOG;   \
                            if (key < array[mid_pos].FIELD_NAME) {                                  \
                                u_idx = mid_pos - 1;                                                \
                            } else {                                                                \
                                if (key > array[mid_pos].FIELD_NAME) {                              \
                                    l_idx = mid_pos + 1;                                            \
                                } else {                                                            \
                                    LOG_DEBUG4 << "The found mid_pos = "                            \
                                            << SSTR(mid_pos) << END_LOG;                            \
                                    found_pos = mid_pos;                                            \
                                    return true;                                                    \
                                }                                                                   \
                            }                                                                       \
                        }                                                                           \
                        return false;                                                               \
                    }

                //NOTE: Do the binary search, note that we do not take care of index
                //underflows as they are signed. Yet, we might want to take into
                //account the overflows, although these are also not that threatening
                //the reason is that the actual array index is TShortId and we use
                //for index iterations a much longer but signed data type TLongId
#define BSEARCH_TWO_FIELDS(FIELD_ONE,FIELD_TWO)                                                     \
                if (DO_SANITY_CHECKS && ((l_idx < 0) || (l_idx > u_idx))) {                         \
                        stringstream msg;                                                           \
                        msg << "Impossible binary search parameters, l_idx = "                      \
                                << SSTR(l_idx) << ", u_idx = "                                      \
                                << SSTR(u_idx) << "!";                                              \
                        throw Exception(msg.str());                                                 \
                    } else {                                                                        \
                        TSLongId mid_pos;                                                           \
                        while (l_idx <= u_idx) {                                                    \
                            mid_pos = (l_idx + u_idx) / 2;                                          \
                            LOG_DEBUG4 << "l_idx = " << SSTR(l_idx) << ", u_idx = "                 \
                                    << SSTR(u_idx) << ", mid_pos = " << SSTR(mid_pos) << END_LOG;   \
                            if (key1 < array[mid_pos].FIELD_ONE) {                                  \
                                u_idx = mid_pos - 1;                                                \
                            } else {                                                                \
                                if (key1 == array[mid_pos].FIELD_ONE) {                                     \
                                    if (key2 < array[mid_pos].FIELD_TWO) {                                  \
                                        u_idx = mid_pos - 1;                                                \
                                    } else {                                                                \
                                        if (key2 == array[mid_pos].FIELD_TWO) {                             \
                                            LOG_DEBUG4 << "The found mid_pos = "                            \
                                                    << SSTR(mid_pos) << END_LOG;                            \
                                            found_pos = mid_pos;                                            \
                                            return true;                                                    \
                                        } else {                                                            \
                                            l_idx = mid_pos + 1;                                            \
                                        }                                                                   \
                                    }                                                                       \
                                } else {                                                                    \
                                    l_idx = mid_pos + 1;                                            \
                                }                                                                   \
                            }                                                                       \
                        }                                                                           \
                        return false;                                                               \
                    }

                /**
                 * This is a binary search algorithm for some ordered array
                 * @param ARR_ELEM_TYPE the array element structure, must have id field as this method will specifically use it to compare elements.
                 * @param IDX_TYPE the index type 
                 * @param KEY_TYPE the key type template parameter
                 * @param COMPARE the compare function template parameter
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param found_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE, typename IDX_TYPE, typename KEY_TYPE,
                typename T_IS_EXT_COMPARE_FUNC<KEY_TYPE>::func_ptr COMPARE_FUNC>
                inline bool my_bsearch_id(const ARR_ELEM_TYPE * array, TSLongId l_idx, TSLongId u_idx,
                        const KEY_TYPE key, IDX_TYPE & found_pos) {
                    LOG_DEBUG3 << "Searching between indexes " << l_idx << " and " << u_idx << END_LOG;
                    if (DO_SANITY_CHECKS && ((l_idx < 0) || (l_idx > u_idx))) {
                        stringstream msg;
                        msg << "Impossible binary search parameters, l_idx = "
                                << SSTR(l_idx) << ", u_idx = "
                                << SSTR(u_idx) << "!";
                        throw Exception(msg.str());
                    } else {
                        TSLongId mid_pos;
                        while (l_idx <= u_idx) {
                            mid_pos = (l_idx + u_idx) / 2;
                            LOG_DEBUG4 << "l_idx = " << SSTR(l_idx) << ", u_idx = "
                                    << SSTR(u_idx) << ", mid_pos = " << SSTR(mid_pos) << END_LOG;
                            int result = COMPARE_FUNC(key, array[mid_pos].id);
                            if (result < 0) {
                                u_idx = mid_pos - 1;
                            } else {
                                if (result == 0) {
                                    LOG_DEBUG4 << "The found mid_pos = "
                                            << SSTR(mid_pos) << END_LOG;
                                    found_pos = (IDX_TYPE) mid_pos;
                                    return true;
                                } else {
                                    l_idx = mid_pos + 1;
                                }
                            }
                        }
                        return false;
                    }
                }

                /**
                 * This is a binary search algorithm for some ordered array for two keys
                 * @param ARR_ELEM_TYPE the array element structure, must have word_id field as this method will specifically use it to compare elements.
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param found_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE>
                inline bool my_bsearch_wordId_ctxId(const ARR_ELEM_TYPE * array, TSLongId l_idx, TSLongId u_idx, const TShortId key1, const TShortId key2, TShortId & found_pos) {
                    BSEARCH_TWO_FIELDS(word_id, ctx_id);
                }

                /**
                 * This is a binary search algorithm for some ordered array
                 * @param ARR_ELEM_TYPE the array element structure, must have ctx_id field as this method will specifically use it to compare elements.
                 * @param IDX_TYPE the index type 
                 * @param KEY_TYPE the key type template parameter
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param found_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE, typename IDX_TYPE, typename KEY_TYPE>
                inline bool my_bsearch_id(const ARR_ELEM_TYPE * array, TSLongId l_idx, TSLongId u_idx, const KEY_TYPE key, IDX_TYPE & found_pos) {
                    BSEARCH_ONE_FIELD(id);
                }

                /**
                 * This is an interpolated search algorithm for some ordered array
                 * WARNING: IS ACTUALLY VERT SLOW at least in the current implementation and for the current application!
                 * @param ARR_ELEM_TYPE the array element structure, must have ctx_id field as this method will specifically use it to compare elements.
                 * @param IDX_TYPE the index type 
                 * @param KEY_TYPE the key type template parameter
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param found_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE, typename IDX_TYPE, typename KEY_TYPE>
                bool my_isearch_id(const ARR_ELEM_TYPE * array, TSLongId l_idx, TSLongId u_idx, const KEY_TYPE key, IDX_TYPE & found_pos) {
                    LOG_DEBUG3 << "Start searching for key: " << (uint32_t) key << " between l_idx: "
                            << l_idx << ", u_idx: " << u_idx << END_LOG;
                    if (DO_SANITY_CHECKS && ((l_idx < 0) || (l_idx > u_idx))) {
                        stringstream msg;
                        msg << "Impossible binary search parameters, l_idx = "
                                << SSTR(l_idx) << ", u_idx = "
                                << SSTR(u_idx) << "!";
                        throw Exception(msg.str());
                    } else {
                        TSLongId mid_pos = 0;
                        while ((array[l_idx].id <= key) && (key <= array[u_idx].id) && (l_idx != u_idx)) {
                            mid_pos = l_idx + ((u_idx - l_idx) * (key - array[l_idx].id)) / (array[u_idx].id - array[l_idx].id);
                            LOG_DEBUG3 << "l_idx:" << l_idx << ", mid_pos:" << mid_pos << ", u_idx:" << u_idx << END_LOG;
                            if (key < array[mid_pos].id) {
                                u_idx = mid_pos - 1;
                            } else {
                                if (key == array[mid_pos].id) {
                                    found_pos = mid_pos;
                                    LOG_DEBUG3 << "Found key: " << (uint32_t) key
                                            << " @ position: " << found_pos << END_LOG;
                                    return true;
                                } else {
                                    l_idx = mid_pos + 1;
                                }
                            }
                        }
                        if (array[l_idx].id == key) {
                            found_pos = l_idx;
                            LOG_DEBUG3 << "Found key: " << (uint32_t) key
                                    << " @ position: " << found_pos << END_LOG;
                            return true;
                        } else {
                            LOG_DEBUG3 << "The key: " << (uint32_t) key
                                    << " was not found!" << END_LOG;
                            return false;
                        }
                    }
                }

                /**
                 * This is a linear search algorithm for some ordered array
                 * @param ARR_ELEM_TYPE the array element structure
                 * @param IDX_TYPE the index type 
                 * @param KEY_TYPE the key type template parameter
                 * @param array the pointer to the first array element
                 * @param l_idx the initial left border index for searching
                 * @param u_idx the initial right border index for searching
                 * @param key the key we are searching for
                 * @param found_pos the out parameter that stores the found element index, if any
                 * @return true if the element was found, otherwise false
                 * @throws Exception in case (l_idx < 0) || (l_idx > u_idx), with sanity checks on
                 */
                template<typename ARR_ELEM_TYPE, typename IDX_TYPE>
                inline bool my_lsearch(const ARR_ELEM_TYPE * array, TSLongId l_idx, TSLongId u_idx, const ARR_ELEM_TYPE key, IDX_TYPE & found_pos) {
                    LOG_DEBUG2 << "Searching between indexes " << l_idx << " and " << u_idx << END_LOG;

                    if (key > array[u_idx]) {
                        LOG_DEBUG3 << "key (+1) value@" << (uint32_t) u_idx << END_LOG;
                        //The key is larger than the last id so it is not in the array
                        return false;
                    } else {
                        if (key == array[u_idx]) {
                            LOG_DEBUG3 << "key (0) value@" << (uint32_t) u_idx << END_LOG;
                            //The key is equal to the last id so we found it!
                            found_pos = u_idx;
                            return true;
                        } else {
                            //The key is potentially inside array and it is not the last element!
                            for (found_pos = l_idx; found_pos < u_idx; ++found_pos) {
                                if (key < array[found_pos]) {
                                    LOG_DEBUG3 << "key (-1) value@" << (uint32_t) found_pos << END_LOG;
                                    //We bypassed the place where the value could have been
                                    return false;
                                } else {
                                    if (key == array[found_pos]) {
                                        LOG_DEBUG3 << "key (0) value@" << (uint32_t) found_pos << END_LOG;
                                        //We found the key!
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                    //The key is outside the array values, no reason to search further
                    return false;
                }

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
                bool my_bsearch(const ARR_ELEM_TYPE * array, INDEX_TYPE l_idx, INDEX_TYPE u_idx, const KEY_TYPE key, INDEX_TYPE & mid_pos) {
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
                                const KEY_TYPE & v_a = *(static_cast<const KEY_TYPE*> (p_a));
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
                 * @param array_begin the pointer to the array's first element
                 * @param array_size the size of the array
                 * @param is_less_func the is-less function
                 */
                template<typename ELEM_TYPE>
                inline void my_sort(ELEM_TYPE * array_begin, const TShortId array_size,
                        typename T_IS_COMPARE_FUNC<ELEM_TYPE>::func_type is_less_func) {
                    //Do not do sorting if the array size is less than two
                    if (array_size > 1) {
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        std::sort(array_begin, array_begin + array_size, is_less_func);
                    }
                }

                /**
                 * This methos is used to do <algorithm> std::sort on an array
                 * of structures convertable to some simple comparable type.
                 * This method does the progress bar update, if needed
                 * @param ELEM_TYPE the array element type
                 * @param IS_LESS_FUNC the is-less function
                 * @param array_begin the pointer to the array's first element
                 * @param array_size the size of the array
                 */
                template<typename ELEM_TYPE, typename T_IS_COMPARE_FUNC<ELEM_TYPE>::func_ptr IS_LESS_FUNC>
                inline void my_sort(ELEM_TYPE * array_begin, const TShortId array_size) {
                    //Do not do sorting if the array size is less than two
                    if (array_size > 1) {
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        std::sort(array_begin, array_begin + array_size, IS_LESS_FUNC);
                    }
                }

                /**
                 * The basic "is less" function for the sort algorithms that allows to update the progress bar.
                 * @param first the first element to compare
                 * @param second the second element to compare
                 * @return true if the first element is less then the second
                 */
                template<typename ELEM_TYPE, bool IS_PROGRESS = true >
                inline bool is_less(const ELEM_TYPE & first, const ELEM_TYPE & second) {
                    if (IS_PROGRESS) {
                        //Update the progress bar status
                        Logger::updateProgressBar();
                    }
                    //Return the result
                    return (first < second);
                }

                /**
                 * This methos is used to do <algorithm> std::sort on an array
                 * of structures convertable to some simple comparable type.
                 * This method does the progress bar update, if needed
                 * @param ELEM_TYPE the array element type
                 * @param IS_PROGRESS if true the progress bar will be updated,
                 * otherwise not, default is true
                 * @param array_begin the pointer to the array's first element
                 * @param array_size the size of the array
                 */
                template<typename ELEM_TYPE, bool IS_PROGRESS = true >
                inline void my_sort(ELEM_TYPE * array_begin, const TShortId array_size) {
                    my_sort<ELEM_TYPE, is_less<ELEM_TYPE, IS_PROGRESS> >(array_begin, array_size);
                }
            }
        }
    }
}

#endif	/* ARRAYUTILS_HPP */

