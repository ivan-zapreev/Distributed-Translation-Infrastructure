/* 
 * File:   MathUtils.hpp
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
 * Created on September 7, 2015, 11:08 AM
 */

#ifndef MATHUTILS_HPP
#define	MATHUTILS_HPP

#include <cstdint>

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::exceptions;

namespace uva {
    namespace utils {
        namespace math {
            namespace log2 {
                //These are the fast log2 computing functions originated from:
                //http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers

                const static uint8_t tab64[64] = {
                    63, 0, 58, 1, 59, 47, 53, 2,
                    60, 39, 48, 27, 54, 33, 42, 3,
                    61, 51, 37, 40, 49, 18, 28, 20,
                    55, 30, 34, 11, 43, 14, 22, 4,
                    62, 57, 46, 52, 38, 26, 32, 41,
                    50, 36, 17, 19, 29, 10, 13, 21,
                    56, 45, 25, 31, 35, 16, 9, 12,
                    44, 24, 15, 8, 23, 7, 6, 5
                };

                static inline uint8_t log2_64(uint64_t value) {
                    value |= value >> 1;
                    value |= value >> 2;
                    value |= value >> 4;
                    value |= value >> 8;
                    value |= value >> 16;
                    value |= value >> 32;
                    return tab64[((uint64_t) ((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
                };

                const static uint8_t tab32[32] = {
                    0, 9, 1, 10, 13, 21, 2, 29,
                    11, 14, 16, 18, 22, 25, 3, 30,
                    8, 12, 20, 28, 15, 17, 24, 7,
                    19, 27, 23, 6, 26, 5, 4, 31
                };

                static inline uint8_t log2_32(uint32_t value) {
                    value |= value >> 1;
                    value |= value >> 2;
                    value |= value >> 4;
                    value |= value >> 8;
                    value |= value >> 16;
                    return tab32[(uint32_t) (value * 0x07C4ACDD) >> 27];
                };
            }

            namespace bits {

                //This array stores the bit copy masks for the unsigned 32-bit integer.
                //These are for the different number of bits to be copied, in the
                //copy_end_bits function.
                const static uint32_t bit_32_copy_masks[] = {
                    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
                    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
                    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
                    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
                    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
                    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
                    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
                    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
                };

                /**
                 * Allows to copy the given number of bits (starting from the end)
                 * into the given bit position of the target byte array
                 * @param source the source 32 bit unsigned integer to copy end bits from
                 * @param num_bits_to_copy the number of bits to copy
                 * @param p_target the byte array to copy bits into, must have sufficient capacity
                 * @param bit_pos the position into which the pits are to be copied
                 */
                static inline void copy_end_bits(const TShortId source, const uint8_t num_bits_to_copy,
                        uint8_t * p_target, const uint32_t bit_pos) {
                    throw Exception("copy_end_bits: Not implemented yet!");
                }
            }
        }
    }
}


#endif	/* MATHUTILS_HPP */

