/* 
 * File:   DynamicMemoryArrays.hpp
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
 * Created on August 27, 2015, 20:17 PM
 */

#include <functional>   // std::function 
#include <cmath>        // std::log std::log10
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "ArrayUtils.hpp"

using namespace std;
using namespace uva::smt::utils::array;

#ifndef DYNAMICMEMORYARRAYS_HPP
#define	DYNAMICMEMORYARRAYS_HPP

namespace uva {
    namespace smt {
        namespace tries {
            namespace alloc {
                /**
                 * This is a function type for the function that should compute the capacity increase
                 * @param the first argument is the current capacity as float
                 * @return the capacity increase
                 */
                typedef std::function<float (const float) > TCapacityIncFunct;

                /**
                 * Stores the string names of the memory increase strategies,
                 * should correspond with the enum MemIncTypesEnum indexes!
                 */
                const char * const _memIncTypesEnumStr[MemIncTypesEnum::size] = {"CONSTANT", "LINEAR", "LOG_2", "LOG_10"};

                /**
                 * This class stores the memory increment strategy and allows to use it
                 */
                class MemIncreaseStrategy {
                private:
                    //Stores the increment strategy type
                    MemIncTypesEnum m_stype;
                    //The field storing the strategy capacity increase function
                    TCapacityIncFunct m_get_capacity_inc_func;
                    //Stores the minimum allowed memory increment, in number of elements
                    size_t m_min_mem_inc;
                    //Stores the maximum allowed memory increment, in percent from the current number of elements
                    float m_mem_inc_factor;

                public:

                    /**
                     * The main constructor to be used
                     * @param stype the strategy type
                     * @param get_capacity_inc_func the strategy function
                     * @param min_mem_inc the minimum memory increase in number of elements
                     * @param mem_inc_factor the memory increment factor, the number we will multiply by the computed increment
                     */
                    MemIncreaseStrategy(const MemIncTypesEnum & stype,
                            const TCapacityIncFunct get_capacity_inc_func,
                            const size_t min_mem_inc, const float mem_inc_factor)
                    : m_stype(stype), m_get_capacity_inc_func(get_capacity_inc_func),
                    m_min_mem_inc(min_mem_inc), m_mem_inc_factor(mem_inc_factor) {
                        if (DO_SANITY_CHECKS && (m_min_mem_inc < 1)) {
                            throw Exception("Inappropriate minimum memory increment!");
                        }
                    }

                    MemIncreaseStrategy(const MemIncreaseStrategy & other)
                    : m_stype(other.m_stype), m_get_capacity_inc_func(other.m_get_capacity_inc_func),
                    m_min_mem_inc(other.m_min_mem_inc), m_mem_inc_factor(other.m_mem_inc_factor) {
                    }

                    /**
                     * Allows to retrieve the strategy name
                     * @return the strategy name
                     */
                    string getStrategyStr() const {
                        stringstream msg;
                        msg << _memIncTypesEnumStr[m_stype] << ", memory increments: Min = "
                                << SSTR(m_min_mem_inc) << " elements, Factor = "
                                << SSTR(m_mem_inc_factor);
                        return msg.str();
                    }

                    /**
                     * Compute the new capacity given the provided one, this function used
                     * the capacity increase function stored in m_get_capacity_inc_func.
                     * @param capacity the current capacity
                     * @return the proposed capacity increase
                     */
                    inline const size_t computeNewCapacity(const size_t capacity) const {
                        //Get the float capacity value, make it minimum of one element to avoid problems
                        const float fcap = (capacity > 0) ? (float) capacity : 1.0;
                        const size_t cap_inc = (size_t) (m_mem_inc_factor * m_get_capacity_inc_func(fcap));
                        return capacity + max(cap_inc, m_min_mem_inc);
                    }
                };

                /**
                 * This is a factory function allowing to ge the strategy object for the given parameters
                 * @param stype the strategy type
                 * @param min_mem_inc the minimum memory increment in number of elements
                 * @param mem_inc_factor the memory increment factor, the number we will multiply by the computed increment
                 * @return the pointer to a newly allocated strategy object
                 */
                inline MemIncreaseStrategy * getMemIncreaseStrategy(const MemIncTypesEnum stype,
                        const size_t min_mem_inc, const float mem_inc_factor) {
                    TCapacityIncFunct inc_func;

                    //ToDo: optimize this switch, it is pretty ugly, use a map or something
                    switch (stype) {
                        case MemIncTypesEnum::CONSTANT:
                            inc_func = [] (const float fcap) -> float {
                                //Return zero as then the minimum constant increase will be used!
                                return 0;
                            };
                            break;
                        case MemIncTypesEnum::LINEAR:
                            inc_func = [] (const float fcap) -> float {
                                return fcap;
                            };
                            break;
                        case MemIncTypesEnum::LOG_2:
                            inc_func = [] (const float fcap) -> float {
                                return fcap / log(fcap);
                            };
                            break;
                        case MemIncTypesEnum::LOG_10:
                            inc_func = [] (const float fcap) -> float {
                                //Get the float capacity value, make it minimum of one element to avoid problems
                                return fcap / log10(fcap);
                            };
                            break;
                        default:
                            stringstream msg;
                            msg << "Unrecognized memory allocation strategy: " << stype;
                            throw Exception(msg.str());
                    }

                    //return the result object
                    return new MemIncreaseStrategy(stype, inc_func, min_mem_inc, mem_inc_factor);
                };

                /**
                 * This class represents a dynamic memory array and stores the main methods needed for its operation
                 */
                template<typename ELEMENT_TYPE>
                class ADynamicStackArray {
                public:

                    /**
                     * The basic constructor, that allows to pre-allocate some memory
                     * @param capacity the initial capacity to allocate
                     */
                    ADynamicStackArray(const size_t capacity)
                    : m_ptr(NULL), m_capacity(0), m_size(0) {
                        //Set the initial capacity and memory strategy via one method
                        pre_allocate(capacity);
                    }

                    /**
                     * The basic constructor, does not pre-allocate any memory
                     */
                    ADynamicStackArray()
                    : m_ptr(NULL), m_capacity(0), m_size(0) {
                    }

                    /**
                     * Allows pre-allocate some capacity
                     * @param capacity the capacity to pre-allocate
                     */
                    inline void pre_allocate(const size_t capacity) {
                        //Reallocate to the desired capacity, if needed
                        if (capacity > m_capacity) {
                            reallocate(capacity);
                        }
                    }

                    /**
                     * Allows to retrieve the next new/unused element.
                     * Reallocates memory, if needed, to get space for the new element
                     * @return the next new element
                     */
                    inline ELEMENT_TYPE & get_new() {
                        //Allocate more memory if needed
                        if (m_size == m_capacity) {
                            reallocate<true>();
                        }
                        //Return the new/free element
                        return m_ptr[m_size++];
                    }

                    /**
                     * De-allocated the un-used memory, if any
                     */
                    inline void shrink() {
                        //If there is space to free, do it
                        if (m_size < m_capacity) {
                            reallocate<false>();
                        }
                    }

                    /**
                     * This operator allows to retrieve the reference to an array element by the given index
                     * @param idx the array element index
                     * @return the reference to the array element under the given index
                     * @throws out_of_range exception if the index is outside the array size.
                     */
                    inline const ELEMENT_TYPE & operator[](TShortId idx) const {
                        if (idx < m_size) {
                            return m_ptr[idx];
                        } else {
                            stringstream msg;
                            msg << "Invalid index: " << SSTR(idx) << ", size: "
                                    << SSTR(m_size) << ", capacity: " << SSTR(m_capacity);
                            throw out_of_range(msg.str());
                        }
                    }

                    /**
                     * Allows to retrieve the currently used number of elements 
                     * @return the number of elements stored in the stack array.
                     */
                    inline size_t get_size() const {
                        return m_size;
                    }

                    /**
                     * Allows to get the pointer to the stored data, note that this
                     * pointer is only guaranteed to be valid until a new element
                     * is added to the array, due to possible memory reallocation
                     * @return the pointer to the data array
                     */
                    inline const ELEMENT_TYPE * get_data() const {
                        return m_ptr;
                    }

                    /**
                     * Allows to check if there is data stored
                     * @return true if there is at least one data element stored otherwise false
                     */
                    inline bool has_data() const {
                        return (m_ptr != NULL) && (m_size > 0);
                    }

                    /**
                     * Allows to sort the data stored in this stack array.
                     * How th data is sorted is defined by the < operator of the ELEMENT_TYPE
                     */
                    inline void sort() {
                        if (m_size > 1) {
                            my_sort<ELEMENT_TYPE>(m_ptr, m_size);
                        }
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~ADynamicStackArray() {
                        if (m_ptr != NULL) {
                            free(m_ptr);
                        }
                    }

                    /**
                     * Allows to get the memory allocation strategy, from the child
                     * @return the memory strategy
                     */
                    virtual const MemIncreaseStrategy * get_mem_strat() = 0;

                private:
                    //The pointer to the stored array elements
                    ELEMENT_TYPE * m_ptr;
                    //Stores the capacity - already allocated memory for this array
                    size_t m_capacity;
                    //Stores the number of used elements, the size of this array
                    size_t m_size;

                    /**
                     * This methods allows to reallocate the data to the new capacity
                     * @param new_capacity the desired new capacity
                     */
                    void reallocate(size_t new_capacity) {
                        LOG_DEBUG2 << "The new capacity is " << SSTR(new_capacity)
                                << ", the old capacity was " << SSTR(m_capacity)
                                << ", used size: " << SSTR(m_size) << END_LOG;

                        //Reallocate memory, potentially we get a new pointer!
                        m_ptr = (ELEMENT_TYPE*) realloc(m_ptr, new_capacity * sizeof (ELEMENT_TYPE));

                        //Clean the newly allocated memory
                        if (new_capacity > m_capacity) {
                            const size_t new_num_elem = (new_capacity - m_capacity);
                            memset(m_ptr + m_capacity, 0, new_num_elem * sizeof (ELEMENT_TYPE));
                        }

                        //Set the new capacity in
                        m_capacity = new_capacity;

                        //Do the null pointer check if sanity
                        if (DO_SANITY_CHECKS && (new_capacity > m_capacity)
                                && (new_capacity > 0) && (m_ptr == NULL)) {
                            stringstream msg;
                            msg << "Ran out of memory when trying to allocate "
                                    << new_capacity << " data elements for a wordId";
                            throw Exception(msg.str());
                        }
                    }

                    /**
                     * Allows to reallocate the word entry data increasing or decreasing its capacity.
                     * How much memory will exactly be allocated depends on the memory increase strategy,
                     * the current capacity and the IS_INC flag.
                     * @param IS_INC if true then the memory will be attempted to increase, otherwise decrease
                     */
                    template<bool IS_INC = true >
                    void reallocate() {
                        size_t new_capacity;

                        LOG_DEBUG2 << "Memory reallocation request: "
                                << ((IS_INC) ? "increase" : "decrease") << END_LOG;

                        //Compute the new number of elements
                        if (IS_INC) {
                            //Compute the new capacity
                            new_capacity = get_mem_strat()->computeNewCapacity(m_capacity);
                            LOG_DEBUG2 << "Computed new capacity is: " << new_capacity << END_LOG;
                        } else {
                            //Decrease the capacity to the current size, remove the unneeded
                            new_capacity = m_size;
                        }

                        //Reallocate to the computed new capacity
                        reallocate(new_capacity);
                    }
                };
            }
        }
    }
}



#endif	/* W2CORDEREDARRAYTRIETRIEMEM_HPP */
