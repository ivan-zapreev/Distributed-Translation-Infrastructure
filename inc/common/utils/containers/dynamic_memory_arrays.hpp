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

#ifndef DYNAMICMEMORYARRAYS_HPP
#define	DYNAMICMEMORYARRAYS_HPP

#include <functional>   // std::function 
#include <cmath>        // std::log std::log10
#include <algorithm>    // std::max

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/containers/array_utils.hpp"
#include "common/utils/math_utils.hpp"

using namespace std;
using namespace uva::utils::containers::utils;
using namespace uva::utils::math::bits;

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * Stores the possible memory increase types
             */
            enum mem_inc_types_enum {
                UNDEFINED = 0,
                CONSTANT = UNDEFINED + 1,
                LINEAR = CONSTANT + 1,
                LOG_2 = LINEAR + 1,
                LOG_10 = LOG_2 + 1,
                size = LOG_10 + 1
            };

            /**
             * This is a function type for the function that should compute the capacity increase
             * @param the first argument is the current capacity as float
             * @return the capacity increase
             */
            typedef std::function<size_t(const size_t) > TCapacityIncFunct;

            /**
             * Stores the string names of the memory increase strategies,
             * should correspond with the enum MemIncTypesEnum indexes!
             */
            const char * const _memIncTypesEnumStr[mem_inc_types_enum::size] = {"CONSTANT", "LINEAR", "LOG_2", "LOG_10"};

            /**
             * This class stores the memory increment strategy and allows to use it
             */
            class mem_increase_strategy {
            private:
                //Stores the increment strategy type
                mem_inc_types_enum m_stype;
                //The field storing the strategy capacity increase function
                TCapacityIncFunct m_get_capacity_inc_func;
                //Stores the minimum allowed memory increment, in number of elements
                size_t m_min_mem_inc;
                //Stores the memory increment coefficient, must be larger than 0
                size_t m_mem_inc_factor;

            public:

                /**
                 * The main constructor to be used
                 * @param stype the strategy type
                 * @param get_capacity_inc_func the strategy function
                 * @param min_mem_inc the minimum memory increase in number of elements
                 * @param mem_inc_factor the memory increment factor, the number we will multiply by the computed increment
                 */
                mem_increase_strategy(const mem_inc_types_enum & stype,
                        const TCapacityIncFunct get_capacity_inc_func,
                        const size_t min_mem_inc, const size_t mem_inc_factor)
                : m_stype(stype), m_get_capacity_inc_func(get_capacity_inc_func),
                m_min_mem_inc(min_mem_inc), m_mem_inc_factor(mem_inc_factor) {
                    //Perform a sanity check if needed.
                    ASSERT_SANITY_THROW((m_min_mem_inc < 1), "Inappropriate minimum memory increment!");
                    ASSERT_SANITY_THROW((m_mem_inc_factor == 0), "Inappropriate memory increment factor!");
                }

                mem_increase_strategy()
                : m_stype(mem_inc_types_enum::UNDEFINED), m_get_capacity_inc_func(NULL),
                m_min_mem_inc(0), m_mem_inc_factor(0) {
                }

                mem_increase_strategy(const mem_increase_strategy & other)
                : m_stype(other.m_stype), m_get_capacity_inc_func(other.m_get_capacity_inc_func),
                m_min_mem_inc(other.m_min_mem_inc), m_mem_inc_factor(other.m_mem_inc_factor) {
                }

                /**
                 * Allows to retrieve the strategy name
                 * @return the strategy name
                 */
                inline string get_strategy_info() const {
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
                inline const size_t get_new_capacity(const size_t capacity) const {
                    //Get the float capacity value, make it minimum of one element to avoid problems
                    const size_t cap_inc = m_mem_inc_factor * m_get_capacity_inc_func((capacity > 0) ? capacity : 1);
                    return capacity + max(cap_inc, m_min_mem_inc);
                }
            };

            /**
             * This is a factory function allowing to ge the strategy object for the given parameters.
             * \todo Optimize the switch, it is pretty ugly, use a map or something.
             * @param stype the strategy type
             * @param min_mem_inc the minimum memory increment in number of elements
             * @param mem_inc_factor the memory increment factor, the number we will multiply by the computed increment
             * @return the pointer to a newly allocated strategy object
             */
            inline mem_increase_strategy get_mem_incr_strat(const mem_inc_types_enum stype,
                    const size_t min_mem_inc, const size_t mem_inc_factor) {
                TCapacityIncFunct inc_func;
                switch (stype) {
                    case mem_inc_types_enum::CONSTANT:
                        inc_func = [] (const size_t fcap) -> size_t {
                            //Return zero as then the minimum constant increase will be used!
                            return 0;
                        };
                        break;
                    case mem_inc_types_enum::LINEAR:
                        inc_func = [] (const size_t fcap) -> size_t {
                            return fcap;
                        };
                        break;
                    case mem_inc_types_enum::LOG_2:
                        inc_func = [] (const size_t fcap) -> size_t {
                            return fcap * (1 / std::log(fcap));
                        };
                        break;
                    case mem_inc_types_enum::LOG_10:
                        inc_func = [] (const size_t fcap) -> size_t {
                            return fcap * (1 / std::log10(fcap));
                        };
                        break;
                    default:
                        stringstream msg;
                        msg << "Unrecognized memory allocation strategy: " << stype;
                        throw uva_exception(msg.str());
                }

                //return the result object
                return mem_increase_strategy(stype, inc_func, min_mem_inc, mem_inc_factor);
            };

            /**
             * The element deallocator function type for the ADynamicStackArray
             */
            template<typename ELEM_TYPE>
            struct ELEMENT_DEALLOC_FUNC {
                typedef std::function<void(ELEM_TYPE &) > func_type;
                typedef void(* func_ptr)(ELEM_TYPE &);
                static constexpr func_ptr NULL_FUNC_PTR = (typename ELEMENT_DEALLOC_FUNC<ELEM_TYPE>::func_ptr)NULL;
            };

            /**
             * This class represents a dynamic memory array and stores the main methods needed for its operation
             * @param ELEMENT_TYPE the array element type
             * @param IDX_DATA_TYPE the type is to be used for the size, capacity and index variables, should be an unsigned type!
             * @param INITIAL_CAPACITY the number of words, which defines the initial capacity.
             * @param DESTRUCTOR the destructor function to be used on the elements when the container is deleted, default is NULL
             */
            template<typename ELEMENT_TYPE, typename IDX_DATA_TYPE, IDX_DATA_TYPE INITIAL_CAPACITY = 0,
            typename ELEMENT_DEALLOC_FUNC<ELEMENT_TYPE>::func_ptr DESTRUCTOR = ELEMENT_DEALLOC_FUNC<ELEMENT_TYPE>::NULL_FUNC_PTR>
            class dynamic_stack_array {
            public:

                //Make the element type publicly available
                typedef ELEMENT_TYPE TElemType;

                //Make the element type publicly available
                typedef IDX_DATA_TYPE TIndexType;

                //The handy typedef for a pointer to the element
                typedef ELEMENT_TYPE * ELEMENT_TYPE_PTR;

                //Stores the maximum value allowed by SIZE_T
                static const size_t MAX_SIZE_TYPE_VALUE;

                //Stores the size of the array where the data is packed
                static constexpr size_t PARAMETERS_SIZE_BYTES = (sizeof (ELEMENT_TYPE_PTR) + 2 * sizeof (IDX_DATA_TYPE));

                /**
                 * The basic constructor, does not pre-allocate any memory
                 */
                dynamic_stack_array() {
                    //Initialize the array
                    memset(m_params, 0, PARAMETERS_SIZE_BYTES);
                    //If the initial capacity is given then pre-allocate data
                    if (INITIAL_CAPACITY > 0) {
                        //Set the initial capacity and memory strategy via one method
                        pre_allocate(INITIAL_CAPACITY);
                    }
                }


#define EXTRACT_P(NAME_PTR)  \
   ELEMENT_TYPE_PTR & NAME_PTR = extract_bytes<sizeof (IDX_DATA_TYPE), ELEMENT_TYPE_PTR > (m_params);

#define EXTRACT_C(NAME_CAPACITY)        \
    IDX_DATA_TYPE & NAME_CAPACITY = extract_bytes<0, IDX_DATA_TYPE > (m_params);

#define EXTRACT_S(NAME_SIZE)    \
    IDX_DATA_TYPE & NAME_SIZE = extract_bytes<sizeof (ELEMENT_TYPE_PTR) + sizeof (IDX_DATA_TYPE), IDX_DATA_TYPE > (m_params);

#define EXTRACT_PC(NAME_PTR, NAME_CAPACITY) \
        EXTRACT_P(NAME_PTR);                \
        EXTRACT_C(NAME_CAPACITY);

#define EXTRACT_PS(NAME_PTR, NAME_SIZE) \
        EXTRACT_P(NAME_PTR);            \
        EXTRACT_S(NAME_SIZE);

#define EXTRACT_PCS(NAME_PTR, NAME_CAPACITY, NAME_SIZE) \
                             EXTRACT_P(NAME_PTR);       \
                             EXTRACT_C(NAME_CAPACITY);  \
                             EXTRACT_S(NAME_SIZE);

                /**
                 * Allows pre-allocate some capacity
                 * @param capacity the capacity to pre-allocate
                 */
                inline void pre_allocate(const IDX_DATA_TYPE capacity) {
                    //Reallocate to the desired capacity, if needed
                    EXTRACT_PC(m_ptr, m_capacity);

                    if (capacity > m_capacity) {
                        reallocate(m_ptr, m_capacity, capacity);
                    }
                }

                /**
                 * Allows to retrieve the next new/unused element.
                 * Reallocates memory, if needed, to get space for the new element
                 * @return the next new element
                 */
                inline ELEMENT_TYPE & allocate() {
                    EXTRACT_PCS(m_ptr, m_capacity, m_size);

                    LOG_DEBUG2 << "Requesting a new DynamicStackArray element, m_size = "
                            << SSTR(m_size) << ", m_capacity = " << SSTR(m_capacity) << END_LOG;

                    //Allocate more memory if needed
                    if (m_size == m_capacity) {
                        const size_t new_capacity = ELEMENT_TYPE::m_mem_strat.get_new_capacity(m_capacity);
                        reallocate<true>(m_ptr, m_capacity, new_capacity);
                    }

                    //Get the reference to the new element
                    ELEMENT_TYPE & new_element = m_ptr[m_size++];

                    //Return the new/free element
                    return new_element;
                }

                /**
                 * De-allocated the un-used memory, if any
                 */
                inline void shrink() {
                    EXTRACT_PCS(m_ptr, m_capacity, m_size);

                    //If there is space to free, do it
                    if (m_size < m_capacity) {
                        reallocate<false>(m_ptr, m_capacity, m_size);
                    }
                }

                /**
                 * This operator allows to retrieve the reference to an array element by the given index
                 * @param idx the array element index
                 * @return the reference to the array element under the given index
                 * @throws out_of_range exception if the index is outside the array size.
                 */
                inline const ELEMENT_TYPE & operator[](IDX_DATA_TYPE idx) const {
                    //Perform sanity checks if needed
                    if (DO_SANITY_CHECKS) {
                        EXTRACT_S(m_size);
                        EXTRACT_C(m_capacity);
                        ASSERT_SANITY_THROW((idx >= m_size), string("Invalid index: ")
                                + std::to_string(idx) + string(", size: ") + std::to_string(m_size)
                                + string(", capacity: ") + std::to_string(m_capacity));
                    }

                    EXTRACT_P(m_ptr);
                    return m_ptr[idx];
                }

                /**
                 * Allows to retrieve the currently used number of elements 
                 * @return the number of elements stored in the stack array.
                 */
                inline IDX_DATA_TYPE size() const {
                    EXTRACT_S(m_size);
                    return m_size;
                }

                /**
                 * Allows to get the pointer to the stored data, note that this
                 * pointer is only guaranteed to be valid until a new element
                 * is added to the array, due to possible memory reallocation
                 * @return the pointer to the data array
                 */
                inline const ELEMENT_TYPE * data() const {
                    EXTRACT_P(m_ptr);
                    return m_ptr;
                }

                /**
                 * Allows to check if there is data stored
                 * @return true if there is at least one data element stored otherwise false
                 */
                inline bool has_data() const {
                    EXTRACT_PS(m_ptr, m_size);
                    return (m_ptr != NULL) && (m_size > 0);
                }

                /**
                 * Allows to sort the data stored in this stack array.
                 * How th data is sorted is defined by the < operator of the ELEMENT_TYPE
                 */
                inline void sort() {
                    EXTRACT_PS(m_ptr, m_size);
                    my_sort<ELEMENT_TYPE>(m_ptr, m_size);
                }

                /**
                 * Allows to sort the data stored in this stack array.
                 * How th data is sorted is defined by the < operator of the ELEMENT_TYPE
                 */
                inline void sort(typename T_IS_COMPARE_FUNC<ELEMENT_TYPE>::func_type is_less_func) {
                    EXTRACT_PS(m_ptr, m_size);
                    my_sort<ELEMENT_TYPE>(m_ptr, m_size, is_less_func);
                }

                /**
                 * The basic destructor
                 */
                ~dynamic_stack_array() {
                    EXTRACT_P(m_ptr);
                    if (m_ptr != NULL) {
                        if (DESTRUCTOR != ELEMENT_DEALLOC_FUNC<ELEMENT_TYPE>::NULL_FUNC_PTR) {
                            EXTRACT_S(m_size);
                            //Call the destructors on the allocated objects
                            for (IDX_DATA_TYPE idx = 0; idx < m_size; ++idx) {
                                LOG_DEBUG4 << "Deallocating an element [" << SSTR(idx)
                                        << "]: " << SSTR((void *) &m_ptr[idx]) << END_LOG;
                                DESTRUCTOR(m_ptr[idx]);
                            }
                        }
                        //Free the allocated arrays
                        free(m_ptr);
                    }
                }

            private:
                //The pointer to the stored array elements
                //static ELEMENT_TYPE * m_ptr;
                //Stores the capacity - already allocated memory for this array
                //static SIZE_T m_capacity;
                //Stores the number of used elements, the size of this array
                //static SIZE_T m_size;

                //A memory efficient storage for the parameters in a form of a byte array
                //The first m_capacity, is then m_ptr, then m_size
                uint8_t m_params[PARAMETERS_SIZE_BYTES];

                /**
                 * Allows to reallocate the word entry data increasing or decreasing its capacity.
                 * How much memory will exactly be allocated depends on the memory increase strategy,
                 * the current capacity and the IS_INC flag.
                 * @param IS_INC if true then the memory will be attempted to increase, otherwise decrease
                 */
                template<bool IS_INC = true >
                static inline void reallocate(ELEMENT_TYPE_PTR & m_ptr, IDX_DATA_TYPE & m_capacity, size_t new_capacity) {
                    LOG_DEBUG2 << "The new capacity is " << SSTR(new_capacity)
                            << ", the old capacity was " << SSTR(m_capacity)
                            << ", ptr: " << SSTR((void*) m_ptr) << ", elem size: "
                            << SSTR(sizeof (ELEMENT_TYPE)) << END_LOG;

                    //Check that there is still space to be allocated
                    ASSERT_CONDITION_THROW(IS_INC && (m_capacity == MAX_SIZE_TYPE_VALUE),
                            string("Unable to increase the capacity, reached the maximum of ") +
                            std::to_string(MAX_SIZE_TYPE_VALUE) + " allowed by the data type!");

                    //Reallocate memory, potentially we get a new pointer!
                    new_capacity = min(new_capacity, MAX_SIZE_TYPE_VALUE);
                    ASSERT_SANITY_THROW((m_capacity == new_capacity), "The new capacity is equal to the old one!");
                    m_ptr = (ELEMENT_TYPE_PTR) realloc(m_ptr, new_capacity * sizeof (ELEMENT_TYPE));

                    LOG_DEBUG2 << "Memory is reallocated ptr: " << SSTR(m_ptr) << END_LOG;

                    //Initialize the newly allocated memory
                    if (IS_INC) {
                        for (IDX_DATA_TYPE idx = m_capacity; idx < new_capacity; ++idx) {
                            new(static_cast<void *> (&m_ptr[idx])) ELEMENT_TYPE();
                            LOG_DEBUG3 << "Creating a new element [" << SSTR(idx)
                                    << "]: " << SSTR((void *) &m_ptr[idx]) << END_LOG;
                        }
                    }

                    //Set the new capacity in
                    m_capacity = new_capacity;

                    LOG_DEBUG2 << "The end capacity is " << SSTR(m_capacity)
                            << ", ptr: " << SSTR(m_ptr) << END_LOG;

                    //Do the null pointer check if sanity
                    ASSERT_SANITY_THROW((m_ptr == NULL),
                            string("Ran out of memory when trying to allocate ") +
                            std::to_string(new_capacity) + " data elements for a word_id");
                }
            };

            //Stores the maximum values of the available unsigned types until and including uint64_t.
            static const size_t MAX_U_TYPE_VALUES[] = {
#ifdef __INT8_TYPE__
                UINT8_MAX,
#else
                0,
#endif
#ifdef __INT16_TYPE__
                UINT16_MAX,
#else
                0,
#endif
#ifdef __INT24_TYPE__
                UINT24_MAX,
#else
                0,
#endif
#ifdef __INT32_TYPE__
                UINT32_MAX,
#else
                0,
#endif
#ifdef __INT40_TYPE__
                UINT40_MAX,
#else
                0,
#endif
#ifdef __INT48_TYPE__
                UINT48_MAX,
#else
                0,
#endif
#ifdef __INT56_TYPE__
                UINT56_MAX,
#else
                0,
#endif
#ifdef __INT64_TYPE__
                UINT64_MAX
#else
                0,
#endif
            };

            //Get the maximum value for the given template type
            template<typename ELEMENT_TYPE, typename IDX_DATA_TYPE, IDX_DATA_TYPE INITIAL_CAPACITY,
            typename ELEMENT_DEALLOC_FUNC<ELEMENT_TYPE>::func_ptr DESTRUCTOR>
            const size_t dynamic_stack_array<ELEMENT_TYPE, IDX_DATA_TYPE, INITIAL_CAPACITY, DESTRUCTOR>::MAX_SIZE_TYPE_VALUE = MAX_U_TYPE_VALUES[sizeof (IDX_DATA_TYPE) - 1];
        }
    }
}



#endif	/* W2CORDEREDARRAYTRIETRIEMEM_HPP */

