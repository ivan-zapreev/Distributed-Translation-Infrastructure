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

#include <cstdint>      //  std::uint8_t std::uint32_t 
#include <cstring>      //  std::memcpy
#include <bitset>       //  std::bitset

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

                //The number of bits in the uint8_t
                static const uint8_t NUM_BITS_IN_UINT_8 = 8;
                //The number of bits in the uint32_t
                static const uint8_t NUM_BITS_IN_UINT_32 = 32;

                //Computes the byte index from the bit position
#define BYTE_IDX(total_bit_pos) ((total_bit_pos) / NUM_BITS_IN_UINT_8)
                //Computes the bit index within a byte from bit position
#define REMAINING_BIT_IDX(total_bit_pos) ((total_bit_pos) % NUM_BITS_IN_UINT_8)

                //Computes the number of full bytes in n bits
#define NUM_FULL_BYTES(number_of_bits) ((number_of_bits) / NUM_BITS_IN_UINT_8)
                //Computes the number of remaining bits if converted to bytes in n bits
#define NUM_BITS_REMAINDER(number_of_bits) ((number_of_bits) % NUM_BITS_IN_UINT_8)
                //Computes the whole number of bytes needed to store the given amount of bits
#define NUM_BITS_TO_STORE_BYTES(number_of_bits) (((number_of_bits) + (NUM_BITS_IN_UINT_8 - 1)) / NUM_BITS_IN_UINT_8)
                //Allows to convert the number of bytes into the number of bits
#define BYTES_TO_BITS(number_of_bytes) ((number_of_bytes) * NUM_BITS_IN_UINT_8)

                /*
                 * Gets a byte with the bit on the given position set to 1, the rest are zero
                 * The array is ordered as if the bits would have a reversed order, this is
                 * needed as we treat them as an array, so we inverse the bit index
                 */
                static uint8_t copy_bits_array[] = {
                    0x00000080, 0x00000040, 0x00000020, 0x00000010,
                    0x00000008, 0x00000004, 0x00000002, 0x00000001
                };

                /**
                 * Allows to copy the bit value within a byte from one byte to another
                 * @param source the source byte
                 * @param sbit_idx the source byte idx to copy from, the bit index should start from 0 and go until 7!
                 * @param target the target byte
                 * @param tbit_idx the target byte idx to copy to, the bit index should start from 0 and go until 7!
                 */
                static inline void copy_one_bit(const uint8_t source, const uint8_t sbit_idx, uint8_t & target, const uint8_t tbit_idx) {
                    LOG_DEBUG4 << "Source bits: " << bitset<NUM_BITS_IN_UINT_8>(source) << END_LOG;
                    LOG_DEBUG4 << "Target bits: " << bitset<NUM_BITS_IN_UINT_8>(target) << END_LOG;

                    if (source & copy_bits_array[sbit_idx]) {
                        LOG_DEBUG4 << "Copying bit " << SSTR((uint32_t) sbit_idx) << ", it is ON" << END_LOG;
                        target |= copy_bits_array[tbit_idx];
                    } else {
                        LOG_DEBUG4 << "Copying bit " << SSTR((uint32_t) sbit_idx) << ", it is OFF" << END_LOG;
                        target &= ~copy_bits_array[tbit_idx];
                    }

                    LOG_DEBUG4 << "Result bits: " << bitset<NUM_BITS_IN_UINT_8>(target) << END_LOG;
                };

                /**
                 * Allows to copy single bits one by one from one byte array to another
                 * @param p_source the byte array to copy from
                 * @param p_target the byte array to copy to (must be of sufficient capacity)
                 * @param from_pos_bit the bit index (left to right) to start copying bits from
                 * @param to_pos_bit  the bit index (left to right) to start placing bits to
                 * @param num_bits the number of bits to copy
                 */
                static inline void copy_single_bits(const uint8_t * p_source, uint32_t from_pos_bit,
                        uint8_t * p_target, uint32_t to_pos_bit, const uint8_t num_bits) {
                    //Copy bits one by one
                    const uint8_t end_pos_bit = (from_pos_bit + num_bits);

                    LOG_DEBUG4 << "from_pos_bit = " << SSTR((uint32_t) from_pos_bit)
                            << ", end_pos_bit = " << SSTR((uint32_t) end_pos_bit) << END_LOG;

                    while (from_pos_bit < end_pos_bit) {
                        LOG_DEBUG4 << "BYTE_IDX(from_pos_bit) = " << SSTR((uint32_t) BYTE_IDX(from_pos_bit))
                                << ", REMAINING_BIT_IDX(from_pos_bit) = " << SSTR((uint32_t) REMAINING_BIT_IDX(from_pos_bit))
                                << ", BYTE_IDX(to_pos_bit) = " << SSTR((uint32_t) BYTE_IDX(to_pos_bit))
                                << ", REMAINING_BIT_IDX(to_pos_bit) = " << SSTR((uint32_t) REMAINING_BIT_IDX(to_pos_bit)) << END_LOG;
                        //Copy one bit
                        copy_one_bit(p_source[BYTE_IDX(from_pos_bit)],
                                REMAINING_BIT_IDX(from_pos_bit),
                                p_target[BYTE_IDX(to_pos_bit)],
                                REMAINING_BIT_IDX(to_pos_bit));

                        //Increment the positions
                        from_pos_bit++;
                        to_pos_bit++;
                    }
                };

                /**
                 * Allows to copy bits from one byte array to another
                 * @param p_source the byte array to copy from
                 * @param p_target the byte array to copy to (must be of sufficient capacity)
                 * @param from_pos_bit the bit index (left to right) to start copying bits from
                 * @param to_pos_bit  the bit index (left to right) to start placing bits to
                 * @param num_bits the number of bits to copy
                 */
                static inline void copy_all_bits(const uint8_t * p_source, uint32_t from_pos_bit,
                        uint8_t * p_target, uint32_t to_pos_bit, const uint8_t num_bits) {

                    //Depending on whether the bits are byte aligned, we have two copying strategies.
                    if ((REMAINING_BIT_IDX(from_pos_bit) > 0) || (REMAINING_BIT_IDX(to_pos_bit) > 0)) {
                        //If there is no byte alignment: Copy bits one by one
                        copy_single_bits(p_source, from_pos_bit, p_target, to_pos_bit, num_bits);
                    } else {
                        //If there is byte alignment then we can fast copy
                        //some whole bytes and then the remaining bits.

                        //1. Copy as many whole bytes as possible
                        const uint8_t num_full_bytes = NUM_FULL_BYTES(num_bits);
                        if (num_full_bytes > 0) {
                            const uint8_t from_pos_byte = BYTE_IDX(from_pos_bit);
                            const uint8_t to_pos_byte = BYTE_IDX(to_pos_bit);
                            memcpy(p_target + to_pos_byte, p_source + from_pos_byte, num_full_bytes);
                        }

                        //2. Copy the remaining bits one by one
                        const uint8_t num_rem_bits = NUM_BITS_REMAINDER(num_bits);
                        if (num_rem_bits > 0) {
                            //Compute the number of remaining bytes to copy
                            const uint8_t num_copied_bits = BYTES_TO_BITS(num_full_bytes);
                            //Copy the remaining bytes
                            copy_single_bits(p_source, from_pos_bit + num_copied_bits,
                                    p_target, to_pos_bit + num_copied_bits,
                                    num_rem_bits);
                        }
                    }
                };

                /**
                 * Allows to copy the given number of bits (starting from the end)
                 * into the given bit position of the target byte array
                 * @param source the source 32 bit unsigned integer to copy end bits from
                 * @param num_bits_to_copy the number of bits to copy
                 * @param p_target the byte array to copy bits into, must have sufficient capacity
                 * @param bit_pos the position into which the pits are to be copied
                 */
                static inline void copy_end_bits_to_pos(uint32_t source, const uint8_t num_bits,
                        uint8_t * p_target, const uint32_t to_pos_bit) {

                    LOG_DEBUG4 << "Copying " << SSTR((uint32_t) num_bits) << " end bits from "
                            << bitset<NUM_BITS_IN_UINT_32>(source)
                            << " to position: " << SSTR(to_pos_bit) << END_LOG;

                    //First transform the source uint into an array of bytes, taking
                    //care of endianness:
                    //https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
                    //https://en.wikipedia.org/wiki/Endianness
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    source = __builtin_bswap32(source);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    //This should work fine, no inversions or transformations are needed
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
                    throw Exception("copy_end_bits_to_pos: Unsupported endian possibly __ORDER_PDP_ENDIAN__?");
#endif
                    const uint8_t * p_source = static_cast<const uint8_t *> (static_cast<const void *> (& source));

                    LOG_DEBUG4 << "Converted source: "
                            << bitset<NUM_BITS_IN_UINT_8>(p_source[0])
                            << bitset<NUM_BITS_IN_UINT_8>(p_source[1])
                            << bitset<NUM_BITS_IN_UINT_8>(p_source[2])
                            << bitset<NUM_BITS_IN_UINT_8>(p_source[3]) << END_LOG;

                    //Compute the position to start copying from
                    const uint8_t from_pos_bit = (NUM_BITS_IN_UINT_32 - num_bits);

                    //Copy the given number of bits from and to defined targets and positions 
                    copy_all_bits(p_source, from_pos_bit, p_target, to_pos_bit, num_bits);
                };

                /**
                 * This function allows to copy the given number of bits from the
                 * beginning of the given byte array into the end of the given
                 * target variable.
                 * @param num_bits the number of bits to copy
                 * @param p_source the byte array with data to copy from
                 * @param target the variable to put the bits at the end of
                 */
                template<uint8_t num_bits>
                static inline void copy_begin_bits_to_end(const uint8_t * p_source, uint32_t & target) {
                    LOG_DEBUG4 << "Copying " << SSTR((uint32_t) num_bits)
                            << " bits to " << bitset<NUM_BITS_IN_UINT_32>(target) << END_LOG;

                    //Convert the id_type storing variable into an array of bytes
                    uint8_t * p_target = static_cast<uint8_t *> (static_cast<void *> (&target));

                    //The position to start copying from
                    const uint8_t from_pos_bit = 0;
                    //The position to start copying to
                    const uint8_t to_pos_bit = (NUM_BITS_IN_UINT_32 - num_bits);

                    //Copy the given number of bits from and to defined targets and positions 
                    copy_all_bits(p_source, from_pos_bit, p_target, to_pos_bit, num_bits);

                    //Now re-order the target uint, taking care of endianness:
                    //https://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
                    //https://en.wikipedia.org/wiki/Endianness
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                    target = __builtin_bswap32(target);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
                    //This should work fine, no inversions or transformations are needed
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
                    throw Exception("copy_end_bits_to_pos: Unsupported endian possibly __ORDER_PDP_ENDIAN__?");
#endif
                };
            }
        }
    }
}


#endif	/* MATHUTILS_HPP */

