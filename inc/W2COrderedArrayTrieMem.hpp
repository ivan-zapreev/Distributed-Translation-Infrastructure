/* 
 * File:   W2COrderedArrayTrieMem.hpp
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

using namespace std;

#ifndef W2CORDEREDARRAYTRIEMEM_HPP
#define	W2CORDEREDARRAYTRIEMEM_HPP

namespace uva {
    namespace smt {
        namespace tries {
            namespace __W2COrderedArrayTrie {
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
                        if (m_min_mem_inc < 1) {
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
                    string getStrategyStr() {
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
                     * @return the proposed capacity increase, will be at least __W2COrderedArrayTrie::MIN_MEM_INC_NUM
                     */
                    inline const size_t computeNewCapacity(const size_t capacity) {
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
                            msg << "Unrecognized memory allocation strategy: " << __W2COrderedArrayTrie::MEM_INC_TYPE;
                            throw Exception(msg.str());
                    }

                    //return the result object
                    return new MemIncreaseStrategy(stype, inc_func, min_mem_inc, mem_inc_factor);
                }
            }
        }
    }
}



#endif	/* W2CORDEREDARRAYTRIETRIEMEM_HPP */

