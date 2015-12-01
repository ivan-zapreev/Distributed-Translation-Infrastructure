/* 
 * File:   ByteMGramId.cpp
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
 * Created on September 14, 2015, 11:10 PM
 */

#include "ByteMGramId.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

using namespace std;

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {

                namespace m_gram_id {

                    //The maximum values to be stored in so many bit data type values
#define MAX_VALUE_IN_BYTES(NUM_BYTES) (const_expr::power(2, BYTES_TO_BITS(NUM_BYTES)) - 1)
                    static constexpr uint64_t MAX_VALUE_UINT8_T = MAX_VALUE_IN_BYTES(1);
                    static constexpr uint64_t MAX_VALUE_UINT16_T = MAX_VALUE_IN_BYTES(2);
                    static constexpr uint64_t MAX_VALUE_UINT24_T = MAX_VALUE_IN_BYTES(3);
                    static constexpr uint64_t MAX_VALUE_UINT32_T = MAX_VALUE_IN_BYTES(4);
                    static constexpr uint64_t MAX_VALUE_UINT40_T = MAX_VALUE_IN_BYTES(5);
                    static constexpr uint64_t MAX_VALUE_UINT48_T = MAX_VALUE_IN_BYTES(6);
                    static constexpr uint64_t MAX_VALUE_UINT56_T = MAX_VALUE_IN_BYTES(7);
                    static constexpr uint64_t MAX_VALUE_UINT64_T = MAX_VALUE_IN_BYTES(8);

                    /**
                     * This method allows to get the number of bytes needed to store this word id
                     * @param word_id the word id to analyze
                     * @return the number of bytes needed to store this word id
                     */
                    static inline uint8_t get_number_of_bytes(const uint64_t word_id) {
                        if (word_id <= MAX_VALUE_UINT8_T) {
                            LOG_DEBUG4 << "# bytes to store " << word_id << " is: 1, as it is <= " << MAX_VALUE_UINT8_T << END_LOG;
                            return 1u;
                        } else {
                            if (word_id <= MAX_VALUE_UINT16_T) {
                                LOG_DEBUG4 << "# bytes to store " << word_id << " is: 2, as it is <= " << MAX_VALUE_UINT16_T << END_LOG;
                                return 2u;
                            } else {
                                if (word_id <= MAX_VALUE_UINT24_T) {
                                    LOG_DEBUG4 << "# bytes to store " << word_id << " is: 3, as it is <= " << MAX_VALUE_UINT24_T << END_LOG;
                                    return 3u;
                                } else {
                                    if (word_id <= MAX_VALUE_UINT32_T) {
                                        LOG_DEBUG4 << "# bytes to store " << word_id << " is: 4, as it is <= " << MAX_VALUE_UINT32_T << END_LOG;
                                        return 4u;
                                    } else {
                                        if (word_id <= MAX_VALUE_UINT40_T) {
                                            LOG_DEBUG4 << "# bytes to store " << word_id << " is: 5, as it is <= " << MAX_VALUE_UINT40_T << END_LOG;
                                            return 5u;
                                        } else {
                                            if (word_id <= MAX_VALUE_UINT48_T) {
                                                LOG_DEBUG4 << "# bytes to store " << word_id << " is: 6, as it is <= " << MAX_VALUE_UINT48_T << END_LOG;
                                                return 6u;
                                            } else {
                                                if (word_id <= MAX_VALUE_UINT56_T) {
                                                    LOG_DEBUG4 << "# bytes to store " << word_id << " is: 7, as it is <= " << MAX_VALUE_UINT56_T << END_LOG;
                                                    return 7u;
                                                } else {
                                                    if (word_id <= MAX_VALUE_UINT64_T) {
                                                        LOG_DEBUG4 << "# bytes to store " << word_id << " is: 8, as it is <= " << MAX_VALUE_UINT64_T << END_LOG;
                                                        return 8u;
                                                    } else {
                                                        throw Exception("uint8_t get_number_of_bytes(const uint64_t word_id): Does not support more than 8 byte values yet!");
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    };

                    /**
                     * Allows to compute the lengths of the word ids of the m-gram
                     * @param word_ids the word ids of the m-gra
                     * @param num_word_ids the number of the word ids
                     * @param len_bytes [out] the length in bytes needed to store each word id
                     * @return the total number of bytes needed to store the given word ids
                     */
                    template<typename TWordIdType>
                    static inline uint8_t compute_byte_lengths(const TWordIdType * word_ids, const TModelLevel num_word_ids, uint8_t * len_bytes) {
                        uint8_t id_len_bytes = 0;

                        //Obtain the word ids and their lengths in bytes and
                        //the total length in bytes needed to store the key
                        for (size_t idx = 0; idx < num_word_ids; ++idx) {
                            //Get the number of bits needed to store this id
                            len_bytes[idx] = get_number_of_bytes(word_ids[idx]);

                            LOG_DEBUG3 << "Word id: " << SSTR(word_ids[idx]) << " len. in bytes: "
                                    << SSTR((uint32_t) len_bytes[idx]) << END_LOG;

                            //Compute the total gram id length in bits
                            id_len_bytes += len_bytes[idx];
                        }

                        return id_len_bytes;
                    }

                    /***********************************************************************************************************************/

                    /**
                     * This method is needed to compute the M-gram id.
                     * 
                     * Let us give an example of a 2-gram id for a given 2-gram:
                     * 
                     * 1) The 2 word_ids are to be converted to the 2-gram id:
                     * There are 4 bytes in one word id and 4 bytes in
                     * another word id, In total we have 4^2 possible 2-gram id
                     * lengths in bytes, if we only use meaningful bytes of the
                     * word id for instance:
                     *       01-01 both really need just two bytes
                     *       01-02 the first needs one and another two
                     *       02-01 the first needs two and another one
                     *       ...
                     *       04-04 both need 8 bytes
                     * 
                     * 2) These 4^2 = 16 combinations uniquely identify the
                     * type of stored id. So this can be an uid of the gram id type.
                     * To store such a uid we need ceil(log2(16)/8)= 1 bits.
                     * 
                     * 3) We create the 2-gram id as a byte array of 1+ bytes: the type
                     * + the meaningful byte from wordId2 and wordId1. We start from
                     * the end (reverse the word order) as this can potentially
                     * increase speed of the comparison operation.
                     */
                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::create_m_gram_id(const TWordIdType * word_ids,
                            const uint8_t num_word_ids, T_Gram_Id_Data_Ptr & m_p_gram_id) {
                        //Do the sanity check if needed
                        ASSERT_SANITY_THROW((num_word_ids < M_GRAM_LEVEL_2) || (num_word_ids > M_GRAM_LEVEL_6),
                                string("create_m_gram_id: Unsupported m-gram level: ") + std::to_string(num_word_ids) +
                                string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) + string(", ") +
                                std::to_string(M_GRAM_LEVEL_6) + string("]"));

                        //Get the id len in bytes
                        const uint8_t TYPE_LEN_BYTES = ID_TYPE_LEN_BYTES[num_word_ids];

                        uint8_t id_len_bytes = TYPE_LEN_BYTES;

                        LOG_DEBUG3 << "Creating the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) TYPE_LEN_BYTES) << END_LOG;

                        //Obtain the word ids and their lengths in bytes and
                        //the total length in bytes needed to store the key
                        uint8_t len_bytes[num_word_ids];
                        id_len_bytes += compute_byte_lengths(word_ids, num_word_ids, len_bytes);

                        LOG_DEBUG3 << "Total len. in bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                        //Allocate the id memory
                        m_p_gram_id = new uint8_t[id_len_bytes];
                        LOG_DEBUG3 << "Created a Byte_M_Gram_Id: " << SSTR((void *) m_p_gram_id) << END_LOG;

                        //NOTE: There is no need to clean the memory!
                        //We do pure assignments of all the bytes
                        //memset(m_p_gram_id, 0, id_len_bytes);

                        //Determine the type id value from the bit lengths of the words
                        const uint32_t & id_type_value = gram_id_byte_len_2_type(num_word_ids, len_bytes);
                        LOG_DEBUG3 << "ID_TYPE_LEN_BYTES: " << (uint32_t) TYPE_LEN_BYTES
                                << ", id_type_value: " << id_type_value << END_LOG;

                        //Set the id type to the very beginning of the M-gram id
                        copy_end_bytes_to_pos(id_type_value, TYPE_LEN_BYTES, m_p_gram_id, 0);

                        //Append the word id meaningful bits to the id in reverse order
                        uint8_t to_byte_pos = TYPE_LEN_BYTES;
                        for (size_t idx = 0; idx < num_word_ids; ++idx) {
                            copy_end_bytes_to_pos(word_ids[idx], len_bytes[idx], m_p_gram_id, to_byte_pos);
                            to_byte_pos += len_bytes[idx];
                        }

                        LOG_DEBUG3 << "Finished making the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) TYPE_LEN_BYTES) << END_LOG;
                        LOG_DEBUG3 << "Id bits: " << bytes_to_bit_string(m_p_gram_id, id_len_bytes) << END_LOG;

                        return id_len_bytes;
                    }

                    /**
                     * This method is needed to compute the M-gram id.
                     * 
                     * Let us give an example of a 2-gram id for a given 2-gram:
                     * 
                     * 1) The 2 word_ids are to be converted to the 2-gram id:
                     * There are 4 bytes in one word id and 4 bytes in
                     * another word id, In total we have 4^2 possible 2-gram id
                     * lengths in bytes, if we only use meaningful bytes of the
                     * word id for instance:
                     *       01-01 both really need just two bytes
                     *       01-02 the first needs one and another two
                     *       02-01 the first needs two and another one
                     *       ...
                     *       04-04 both need 8 bytes
                     * 
                     * 2) These 4^2 = 16 combinations uniquely identify the
                     * type of stored id. So this can be an uid of the gram id type.
                     * To store such a uid we need ceil(log2(16)/8)= 1 bits.
                     * 
                     * 3) We create the 2-gram id as a byte array of 1+ bytes: the type
                     * + the meaningful byte from wordId2 and wordId1. We start from
                     * the end (reverse the word order) as this can potentially
                     * increase speed of the comparison operation.
                     */
                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::compute_m_gram_id(const TWordIdType * word_ids,
                            const uint8_t num_word_ids, T_Gram_Id_Data_Ptr m_p_gram_id) {
                        //Do the sanity check if needed
                        ASSERT_SANITY_THROW((num_word_ids < M_GRAM_LEVEL_2) || (num_word_ids > M_GRAM_LEVEL_6),
                                string("create_m_gram_id: Unsupported m-gram level: ") + std::to_string(num_word_ids) +
                                string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) + string(", ") +
                                std::to_string(M_GRAM_LEVEL_6) + string("]"));

                        //Get the id len in bytes
                        const uint8_t TYPE_LEN_BYTES = ID_TYPE_LEN_BYTES[num_word_ids];

                        LOG_DEBUG3 << "Creating the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) TYPE_LEN_BYTES) << END_LOG;

                        //Obtain the word ids and their lengths in bytes and
                        //the total length in bytes needed to store the key
                        uint8_t len_bytes[num_word_ids];
                        uint8_t id_len_bytes = TYPE_LEN_BYTES;

                        //Obtain the word ids and their lengths in bytes and
                        //the total length in bytes needed to store the key
                        for (size_t idx = 0; idx < num_word_ids; ++idx) {
                            //Get the number of bits needed to store this id
                            len_bytes[idx] = get_number_of_bytes(word_ids[idx]);
                            //Copy the bytes to the m-gram id
                            copy_end_bytes_to_pos(word_ids[idx], len_bytes[idx], m_p_gram_id, id_len_bytes);
                            //Compute the total gram id length in bytes
                            id_len_bytes += len_bytes[idx];
                            LOG_DEBUG3 << "Word id: " << SSTR(word_ids[idx]) << " len. in bytes: "
                                    << SSTR((uint32_t) len_bytes[idx]) << END_LOG;
                        }

                        LOG_DEBUG3 << "Total len. in bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                        //Determine the type id value from the bit lengths of the words
                        const uint32_t & id_type_value = gram_id_byte_len_2_type(num_word_ids, len_bytes);
                        LOG_DEBUG3 << "ID_TYPE_LEN_BYTES: " << (uint32_t) TYPE_LEN_BYTES
                                << ", id_type_value: " << id_type_value << END_LOG;

                        //Set the id type to the very beginning of the M-gram id
                        copy_end_bytes_to_pos(id_type_value, TYPE_LEN_BYTES, m_p_gram_id, 0);

                        LOG_DEBUG3 << "Finished making the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) TYPE_LEN_BYTES) << END_LOG;
                        LOG_DEBUG3 << "Id bits: " << bytes_to_bit_string(m_p_gram_id, id_len_bytes) << END_LOG;

                        return id_len_bytes;
                    }

                    //Make sure at least the following templates are instantiated
                    template class Byte_M_Gram_Id<uint32_t, M_GRAM_LEVEL_MAX>;
                    template class Byte_M_Gram_Id<uint64_t, M_GRAM_LEVEL_MAX>;
                }
            }
        }
    }
}

