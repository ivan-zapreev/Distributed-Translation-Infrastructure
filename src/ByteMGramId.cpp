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
                     * This method is needed to compute the id type identifier.
                     * Can compute the id type for M-grams until (including) M = 5
                     * The type is computed as in a 32-based numeric system, e.g. for M==5:
                     *          (len_bits[0]-1)*32^0 + (len_bits[1]-1)*32^1 +
                     *          (len_bits[2]-1)*32^2 + (len_bits[3]-1)*32^3 +
                     *          (len_bits[4]-1)*32^4
                     * @param gram_level the number of word ids
                     * @param len_bytes the bytes needed per word id
                     * @param id_type [out] the resulting id type the initial value is expected to be 0
                     */
                    template<typename TWordIdType>
                    inline void gram_id_byte_len_2_type(const TModelLevel gram_level, uint8_t * len_bytes, uint32_t & id_type) {
                        //Do the sanity check for against overflows
                        ASSERT_SANITY_THROW((gram_level > M_GRAM_LEVEL_5), string("Unsupported m-gram level: ") +
                                std::to_string(gram_level) + string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) +
                                string(", ") + std::to_string(M_GRAM_LEVEL_6) + string("], insufficient multipliers!"));

                        LOG_DEBUG3 << "Computing the " << SSTR(gram_level) << "-gram id type" << END_LOG;

                        //Compute the M-gram id type. Here we use the pre-computed multipliers
                        for (size_t idx = 0; idx < gram_level; ++idx) {
                            LOG_DEBUG3 << ((uint32_t) len_bytes[idx] - 1) << " * " << Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx] << " =  "
                                    << ((uint32_t) len_bytes[idx] - 1) * Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx] << END_LOG;

                            id_type += ((uint32_t) len_bytes[idx] - 1) * Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx];
                        }
                        LOG_DEBUG3 << "Resulting id_type = " << SSTR(id_type) << END_LOG;
                    };

                    /**
                     * Allows to compute the byte length for the id of the given type
                     * @param gram_level the M-Gram level M
                     * @param id_type the type id
                     * @param id_len_bytes [out] the total byte length to store the id of this type
                     */
                    template<typename TWordIdType>
                    inline void gram_id_type_2_byte_len(const TModelLevel gram_level, uint32_t id_type, uint8_t & id_len_bytes) {
                        //2. Compute the M-gram id length from the M-gram id type.
                        //   Here we use the pre-computed multipliers we add the
                        //   final bits at the end of the function.
                        uint8_t coeff = 0;
                        for (int idx = (gram_level - 1); idx >= 0; --idx) {

                            coeff = (uint8_t) (id_type / Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx]);

                            LOG_DEBUG3 << SSTR(id_type) << " / " << SSTR(Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx]) << " =  " << SSTR((uint32_t) coeff) << END_LOG;

                            id_type = (uint8_t) (id_type % Byte_M_Gram_Id<TWordIdType>::gram_id_type_mult[idx]);
                            id_len_bytes += coeff;
                        }
                        //Note that in the loop above we have "coeff = len_bits[idx] - 1"
                        //Therefore, here we add the number of tokens to account for this -1's
                        id_len_bytes += (uint8_t) gram_level;
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
                     * 
                     * @param id_type_len_bytes the number of bytes needed to store the M-gram type
                     * @param num_word_ids the number of tokens in the M-gram
                     * @param word_ids the pointer to the array of word ids
                     * @param m_p_gram_id the pointer to the data storage to be initialized.
                     * If (*m_p_p_gram_id == NULL) new memory will be allocated for the id, otherwise the
                     * pointer will be used to store the id. It is then assumed that there is enough memory
                     * allocated to store the id. For an M-gram it is (4*M) bytes that is needed to store
                     * the longed M-gram id.
                     */
                    template<typename TWordIdType>
                    static inline void create_gram_id(const TWordIdType * word_ids, const TModelLevel num_word_ids,
                            T_Gram_Id_Data_Ptr & m_p_gram_id, const uint8_t id_type_len_bytes) {
                        uint8_t id_len_bytes = id_type_len_bytes;

                        LOG_DEBUG3 << "Creating the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) id_type_len_bytes) << END_LOG;

                        //Obtain the word ids and their lengths in bytes and
                        //the total length in bytes needed to store the key
                        uint8_t len_bytes[num_word_ids];
                        for (size_t idx = 0; idx < num_word_ids; ++idx) {
                            //Get the number of bits needed to store this id
                            len_bytes[idx] = get_number_of_bytes(word_ids[idx]);

                            LOG_DEBUG3 << "Word id: " << SSTR(word_ids[idx]) << " len. in bytes: "
                                    << SSTR((uint32_t) len_bytes[idx]) << END_LOG;

                            //Compute the total gram id length in bits
                            id_len_bytes += len_bytes[idx];
                        }

                        LOG_DEBUG3 << "Total len. in bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                        //Allocate the id memory if there was nothing pre-allocated yet
                        if (m_p_gram_id == NULL) {
                            //Allocate memory
                            m_p_gram_id = new uint8_t[id_len_bytes];
                            LOG_DEBUG3 << "Created a Byte_M_Gram_Id: " << SSTR((void *) m_p_gram_id) << END_LOG;
                        }
                        //Clean the memory
                        memset(m_p_gram_id, 0, id_len_bytes);

                        //Determine the type id value from the bit lengths of the words
                        uint32_t id_type_value = 0;
                        gram_id_byte_len_2_type<TWordIdType>(num_word_ids, len_bytes, id_type_value);
                        LOG_DEBUG3 << "ID_TYPE_LEN_BYTES: " << (uint32_t) id_type_len_bytes
                                << ", id_type_value: " << id_type_value << END_LOG;

                        //Append the id type to the M-gram id
                        uint8_t to_byte_pos = 0;
                        copy_end_bytes_to_pos(id_type_value, id_type_len_bytes, m_p_gram_id, to_byte_pos);
                        to_byte_pos += id_type_len_bytes;

                        //Append the word id meaningful bits to the id in reverse order
                        for (int idx = (num_word_ids - 1); idx >= 0; --idx) {

                            copy_end_bytes_to_pos(word_ids[idx], len_bytes[idx], m_p_gram_id, to_byte_pos);
                            to_byte_pos += len_bytes[idx];
                        }

                        LOG_DEBUG3 << "Finished making the " << SSTR(num_word_ids) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) id_type_len_bytes)
                                << ", bits: " << bytes_to_bit_string(m_p_gram_id, id_len_bytes) << END_LOG;
                    };

                    /***********************************************************************************************************************/

                    template<typename TWordIdType>
                    void Byte_M_Gram_Id<TWordIdType>::create_m_gram_id(const TWordIdType * word_ids, const uint8_t num_word_ids, T_Gram_Id_Data_Ptr & m_p_gram_id) {

                        ASSERT_SANITY_THROW((num_word_ids < M_GRAM_LEVEL_2) || (num_word_ids > M_GRAM_LEVEL_5),
                                string("create_m_gram_id: Unsupported m-gram level: ") + std::to_string(num_word_ids) +
                                string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) + string(", ") +
                                std::to_string(M_GRAM_LEVEL_5) + string("]"));

                        //Get the id len in bytes
                        static const uint8_t ID_TYPE_LEN_BYTES = M_GRAM_ID_TYPE_LEN_BYTES[num_word_ids];

                        //Call the appropriate function, use array instead of switch, should be faster.
                        m_gram_id::create_gram_id(word_ids, num_word_ids, m_p_gram_id, ID_TYPE_LEN_BYTES);
                    };

                    template<typename TWordIdType>
                    int Byte_M_Gram_Id<TWordIdType>::compare(const TModelLevel num_words, const T_Gram_Id_Data_Ptr & m_p_gram_id_one, const T_Gram_Id_Data_Ptr & m_p_gram_id_two) {
                        //Do the sanity check if needed
                        if (DO_SANITY_CHECKS && (num_words > M_GRAM_LEVEL_6)) {
                            stringstream msg;
                            msg << "get_gram_id_len: Unsupported m-gram level: "
                                    << SSTR(num_words) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_6) << "], need to set proper type len!";
                            throw Exception(msg.str());
                        }

                        //Get the id len in bytes
                        static const uint8_t ID_TYPE_LEN_BYTES = M_GRAM_ID_TYPE_LEN_BYTES[num_words];

                        //Get the M-gram type ids
                        TShortId type_one = 0;
                        copy_begin_bytes_to_end(m_p_gram_id_one, ID_TYPE_LEN_BYTES, type_one);
                        TShortId type_two = 0;
                        copy_begin_bytes_to_end(m_p_gram_id_two, ID_TYPE_LEN_BYTES, type_two);

                        LOG_DEBUG3 << "M_GRAM_LEVEL: " << (uint32_t) num_words << ", type_one: " << type_one << ", type_two: " << type_two << END_LOG;

                        if (type_one < type_two) {
                            //The first id type is smaller
                            LOG_DEBUG3 << (void*) m_p_gram_id_one << " < " << (void*) m_p_gram_id_two << END_LOG;
                            return -1;
                        } else {
                            if (type_one > type_two) {
                                //The second id type is smaller
                                LOG_DEBUG3 << (void*) m_p_gram_id_one << " > " << (void*) m_p_gram_id_two << END_LOG;
                                return +1;
                            } else {
                                //The id types are the same! Compare the ids themselves
                                LOG_DEBUG3 << (void*) m_p_gram_id_one << " =(type)= " << (void*) m_p_gram_id_two << END_LOG;

                                //Get one of the lengths, as they both are the same
                                uint8_t id_len_bytes = 0;
                                m_gram_id::gram_id_type_2_byte_len<TWordIdType>(num_words, type_one, id_len_bytes);

                                //Start comparing the ids but not from the fist bytes as
                                //this is where the id type information is stored, start
                                //with the first byte that contains key information
                                const uint8_t num_bytes_to_skip = ID_TYPE_LEN_BYTES;

                                LOG_DEBUG3 << "ID_TYPE_LEN_BYTES: " << SSTR((uint32_t) ID_TYPE_LEN_BYTES)
                                        << ", start comparing id key from idx: " << SSTR((uint32_t) num_bytes_to_skip)
                                        << ", Total id_len_bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                                LOG_DEBUG3 << (void*) m_p_gram_id_one << " = " << bytes_to_bit_string(m_p_gram_id_one + num_bytes_to_skip, id_len_bytes) << END_LOG;
                                LOG_DEBUG3 << (void*) m_p_gram_id_two << " = " << bytes_to_bit_string(m_p_gram_id_two + num_bytes_to_skip, id_len_bytes) << END_LOG;

                                //Compare with the fast system function
                                return memcmp(m_p_gram_id_one + num_bytes_to_skip, m_p_gram_id_two + num_bytes_to_skip, id_len_bytes);
                            }
                        }
                    };

                    //Make sure at least the following templates are instantiated
                    template class Byte_M_Gram_Id<uint32_t>;
                    template class Byte_M_Gram_Id<uint64_t>;
                }
            }
        }
    }
}

