/* 
 * File:   ByteMGramId.hpp
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
 * Created on September 14, 2015, 10:43 AM
 */

#ifndef BYTEMGRAMID_HPP
#define	BYTEMGRAMID_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

using namespace std;
using namespace uva::utils::math;
using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {
                /**
                 * This namespace stores some generic macros and functions for the m-gram id
                 * The ones stored here are not made part of the byte-m-gram-id structure as 
                 * they are generic and can be used in other m-gram ids.
                 */
                namespace m_gram_id {

                    //Define the basic type as an alias for the compressed M-Gram id
                    typedef uint8_t * TM_Gram_Id_Value_Ptr;

#pragma pack(push, 1) // exact fit - no padding

                    //Define the m-gram id as a structure storing two things
                    //The m-gram id type and the pointer to the m-gram id itself
                    typedef struct {
                         uint32_t id_type;
                         TM_Gram_Id_Value_Ptr id_value;
                    } TM_Gram_Id;

                    /**
                     * This structure defined the m-gram id key which consists of the m-gram id and its length in bytes
                     */
                    typedef struct {
                        TM_Gram_Id_Value_Ptr m_id;
                        uint8_t m_len_bytes;
                    } T_Gram_Id_Key;

#pragma pack(pop) //back to whatever the previous packing mode was 

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_m_gram_id(TM_Gram_Id_Value_Ptr & m_p_gram_id, uint8_t size) {
                        //Allocate maximum memory that could be needed to store the given M-gram level id
                        m_p_gram_id = new uint8_t[size];
                        LOG_DEBUG4 << "Allocating a M_Gram_Id: " << (void*) m_p_gram_id << " of size " << (uint32_t) size << END_LOG;
                    }

                    /**
                     * Allows to destroy the M-Gram id if it is not NULL.
                     * @param m_p_gram_id the M-gram id pointer to destroy
                     */
                    static inline void destroy(TM_Gram_Id_Value_Ptr & m_p_gram_id) {
                        if (m_p_gram_id != NULL) {
                            LOG_DEBUG4 << "Deallocating a M_Gram_Id: " << (void*) m_p_gram_id << END_LOG;
                            delete[] m_p_gram_id;
                        }
                    }

                    /**
                     * The byte-compressed implementation of the M-gram id class
                     */
                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    class Byte_M_Gram_Id {
                    public:

                        //Stores ther number of bytes needed to store a word id
                        static constexpr uint8_t NUM_BYTES_WORD_ID = sizeof (TWordIdType);

                        /**
                         * Stores the m-gram id multipliers multipliers up to and including level 7
                         */
                        static constexpr uint32_t NUMBER_ID_TYPES_PER_LEVEL[] = {
                            const_expr::power(NUM_BYTES_WORD_ID, 0),
                            const_expr::power(NUM_BYTES_WORD_ID, 1),
                            const_expr::power(NUM_BYTES_WORD_ID, 2),
                            const_expr::power(NUM_BYTES_WORD_ID, 3),
                            const_expr::power(NUM_BYTES_WORD_ID, 4),
                            const_expr::power(NUM_BYTES_WORD_ID, 5),
                            const_expr::power(NUM_BYTES_WORD_ID, 6),
                            const_expr::power(NUM_BYTES_WORD_ID, 7)
                        };

                        //Allows to compute the byte length of the id type if the word ids are of
                        //type TWordIdType and there is NUMBER of them. For example if TWordIdType
                        //is uint64_t (8 bytes) and there is 7 word ids of that type then:
                        //Possible id types: 8^7 = 2,097,152
                        //The number of bits needed to store the type is ceil(log_2(2,097,152)/8) = 3
#define N_GRAM_ID_TYPE_LEN_BYTES(LEVEL) VALUE_LEN_BYTES(NUMBER_ID_TYPES_PER_LEVEL[(LEVEL)])

                        //The length of the M-gram id types in bits depending on the M-Gram level starting from 1.
                        static constexpr uint8_t ID_TYPE_LEN_BYTES[] = {
                            0,
                            N_GRAM_ID_TYPE_LEN_BYTES(1),
                            N_GRAM_ID_TYPE_LEN_BYTES(2),
                            N_GRAM_ID_TYPE_LEN_BYTES(3),
                            N_GRAM_ID_TYPE_LEN_BYTES(4),
                            N_GRAM_ID_TYPE_LEN_BYTES(5),
                            N_GRAM_ID_TYPE_LEN_BYTES(6),
                            N_GRAM_ID_TYPE_LEN_BYTES(7)
                        };

                        //Allows to compute the byte length of the N-gram id if the word ids are of
                        //type TWordIdType and there is NUMBER of them. For example if TWordIdType
                        //Is uint64_t and there is 7 word ids of that type then:
                        //7 TWordIdType values for 7 word ids, plus the memory needed to store type
#define MAX_N_GRAM_ID_LEN_BYTES(LEVEL)  static_cast<uint8_t> ((LEVEL) * NUM_BYTES_WORD_ID + ID_TYPE_LEN_BYTES[LEVEL])

                        //Stores the maximum number of bits up to and including M-grams
                        //of level 7.  We use sizeof (TWordIdType) as each word_id is of type
                        //TWordIdType, and the maximum number of bits is thus defined by the
                        //number of word_ids in the M-gram and their max size in bytes.
                        static constexpr uint8_t MAX_ID_LEN_BYTES[] = {
                            0,
                            MAX_N_GRAM_ID_LEN_BYTES(1),
                            MAX_N_GRAM_ID_LEN_BYTES(2),
                            MAX_N_GRAM_ID_LEN_BYTES(3),
                            MAX_N_GRAM_ID_LEN_BYTES(4),
                            MAX_N_GRAM_ID_LEN_BYTES(5),
                            MAX_N_GRAM_ID_LEN_BYTES(6),
                            MAX_N_GRAM_ID_LEN_BYTES(7)
                        };

                        //Allows to declare the stack allocated m-gram id for the given level and with the given name
#define DECLARE_STACK_GRAM_ID(type, name, level) uint8_t name[type::MAX_ID_LEN_BYTES[(level)]];

                        //Stores the mapping from the m-gram id lengths into the id type value, is pre-computed
#include "ByteMGramIdTables.hpp"

                        /**
                         * This method allows to create new M-gram id for the given M-gram.
                         * There should be no memory allocated for the M-gram id. This method
                         * will allocate as much as needed to store the given id.
                         * @param word_ids the pointer to the array of word ids
                         * @param num_word_ids the number of word ids
                         * @param m_p_gram_id the pointer to the data storage to be initialized
                         * @return the number of bytes in the m-gram id
                         */
                        static uint8_t create_m_gram_id(const TWordIdType * word_ids,
                                const uint8_t num_word_ids, TM_Gram_Id_Value_Ptr & m_p_gram_id);

                        /**
                         * This method allows to compute the m-gram id and set it
                         * into the pre-allocated memory given by a pointer.
                         * @param word_ids the pointer to the array of word ids
                         * @param num_word_ids the number of word ids
                         * @param m_p_gram_id the pointer to the data storage to be initialized
                         * @return the number of bytes in the m-gram id
                         */
                        static uint8_t compute_m_gram_id(const TWordIdType * word_ids,
                                const uint8_t num_word_ids, TM_Gram_Id_Value_Ptr m_p_gram_id);

                        /**
                         * The basic constructor that allocates maximum memory
                         * needed to store the M-gram id of the given level.
                         * @param level the level of the M-grams this object will store id for.
                         * @param m_p_gram_id the pointer to initialize
                         */
                        static inline void allocate_byte_m_gram_id(const TModelLevel level, TM_Gram_Id_Value_Ptr & m_p_gram_id) {
                            //Do the sanity check for against overflows
                            ASSERT_SANITY_THROW((level > M_GRAM_LEVEL_6),
                                    string("Byte_M_Gram_Id: Unsupported m-gram level: ")
                                    + std::to_string(level) + string(", must be within [")
                                    + std::to_string(M_GRAM_LEVEL_2) + string(", ")
                                    + std::to_string(M_GRAM_LEVEL_6) + string("], see M_GRAM_MAX_ID_LEN_BYTES array!"));

                            //Allocate maximum memory that could be needed to store the given M-gram level id
                            m_gram_id::allocate_m_gram_id(m_p_gram_id, MAX_ID_LEN_BYTES[level]);
                        }

                        /**
                         * Allows to compare two M-Gram ids of a fixed M-gram level
                         * @param id_len_bytes the minimum total number of bytes in both m-gram ids.
                         * @param m_p_gram_id_one the first M-gram id
                         * @param m_p_gram_id_two the second M-gram id
                         * @return Negative value if one is smaller than two
                         *         Zero if one is equal to two
                         *         Positive value if one is larger than two
                         */
                        static inline int compare(const uint8_t id_len_bytes,
                                const TM_Gram_Id_Value_Ptr & m_p_gram_id_one,
                                const TM_Gram_Id_Value_Ptr & m_p_gram_id_two) {
                            //Compare with the fast system function. Note that, the
                            //First element of an id is its type, and all of the type
                            //lengths for the m-gram of a given level is the same.
                            //The id type defines the lengths of the rest of the id.
                            //Thus, if the m-gram level is equal then we only get to
                            //compare the bytes after the id type if the id types are equal.
                            LOG_DEBUG2 << "Comparing: " << bytes_to_bit_string(m_p_gram_id_one, id_len_bytes) << END_LOG;
                            LOG_DEBUG2 << "With:      " << bytes_to_bit_string(m_p_gram_id_two, id_len_bytes) << END_LOG;
                            return memcmp(m_p_gram_id_one, m_p_gram_id_two, id_len_bytes);
                        }

                        /**
                         * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                         * @param id_len_bytes the minimum total number of bytes in both m-gram ids.
                         * @param one the first M-gram to compare
                         * @param two the second M-gram to compare
                         * @return true if the first M-gram is "smaller" than the second, otherwise false
                         */
                        static inline bool is_equal_m_grams_id(const uint8_t id_len_bytes,
                                const TM_Gram_Id_Value_Ptr & one, const TM_Gram_Id_Value_Ptr & two) {
                            return (compare(id_len_bytes, one, two) == 0);
                        }

                        /**
                         * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                         * @param id_len_bytes the minimum total number of bytes in both m-gram ids.
                         * @param one the first M-gram to compare
                         * @param two the second M-gram to compare
                         * @return true if the first M-gram is "smaller" than the second, otherwise false
                         */
                        static inline bool is_less_m_grams_id(const uint8_t id_len_bytes,
                                const TM_Gram_Id_Value_Ptr & one, const TM_Gram_Id_Value_Ptr & two) {
                            return (compare(id_len_bytes, one, two) < 0);
                        }

                        /**
                         * Allows to compute the byte length for the id of the given type.
                         * Can compute the byte length for the M-grams until ( and including) M = 6.
                         * @param CURR_LEVEL the M-Gram level M
                         * @param id_type the type id
                         * @return the total byte length to store the id of this type.
                         */
                        template<TModelLevel CURR_LEVEL>
                        static inline const uint8_t & gram_id_type_2_byte_len(uint32_t id_type) {
                            LOG_DEBUG3 << "Computing the " << SSTR(CURR_LEVEL) << "-gram id len in bytes" << END_LOG;

                            //Depending on the level return the pre-computed value
                            switch (CURR_LEVEL) {
                                case M_GRAM_LEVEL_5:
                                    return LEVEL_5_GRAM_TO_BYTE_LEN[id_type];
                                case M_GRAM_LEVEL_4:
                                    return LEVEL_4_GRAM_TO_BYTE_LEN[id_type];
                                case M_GRAM_LEVEL_3:
                                    return LEVEL_3_GRAM_TO_BYTE_LEN[id_type];
                                case M_GRAM_LEVEL_2:
                                    return LEVEL_2_GRAM_TO_BYTE_LEN[id_type];
                                case M_GRAM_LEVEL_6:
                                    return LEVEL_6_GRAM_TO_BYTE_LEN[id_type];
                                default:
                                    THROW_EXCEPTION(string("Unsupported m-gram level: ") + std::to_string(CURR_LEVEL) +
                                            string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) + string(", ") +
                                            std::to_string(M_GRAM_LEVEL_6) + string("], insufficient data!"));
                            }
                        }

                        /**
                         * This method is needed to compute the id type identifier.
                         * Can compute the id type for the M-grams until (and including) M = 6
                         * @param gram_level the number of word ids
                         * @param len_bytes the bytes needed per word id
                         * @param return the resulting id type the initial value is expected to be 0
                         */
                        static inline const uint32_t & gram_id_byte_len_2_type(const TModelLevel gram_level, uint8_t * len_bytes) {
                            LOG_DEBUG3 << "Computing the " << SSTR(gram_level) << "-gram id type" << END_LOG;

                            //Depending on the level return the pre-computed value
                            switch (gram_level) {
                                case M_GRAM_LEVEL_5:
                                    return LEVEL_5_GRAM_TO_TYPE_LEN[len_bytes[0]][len_bytes[1]][len_bytes[2]][len_bytes[3]][len_bytes[4]];
                                case M_GRAM_LEVEL_4:
                                    return LEVEL_4_GRAM_TO_TYPE_LEN[len_bytes[0]][len_bytes[1]][len_bytes[2]][len_bytes[3]];
                                case M_GRAM_LEVEL_3:
                                    return LEVEL_3_GRAM_TO_TYPE_LEN[len_bytes[0]][len_bytes[1]][len_bytes[2]];
                                case M_GRAM_LEVEL_2:
                                    return LEVEL_2_GRAM_TO_TYPE_LEN[len_bytes[0]][len_bytes[1]];
                                case M_GRAM_LEVEL_6:
                                    return LEVEL_6_GRAM_TO_TYPE_LEN[len_bytes[0]][len_bytes[1]][len_bytes[2]][len_bytes[3]][len_bytes[4]][len_bytes[5]];
                                default:
                                    THROW_EXCEPTION(string("Unsupported m-gram level: ") + std::to_string(gram_level) +
                                            string(", must be within [") + std::to_string(M_GRAM_LEVEL_2) + string(", ") +
                                            std::to_string(M_GRAM_LEVEL_6) + string("], insufficient data!"));
                            }
                        };

                        /**
                         * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                         * @param id_type_len_bytes the minimum total number of bytes in both m-gram ids.
                         * @param one the first M-gram to compare
                         * @param two the second M-gram to compare
                         * @return true if the first M-gram is "smaller" than the second, otherwise false
                         */
                        template<TModelLevel CURR_LEVEL>
                        static inline bool is_less_m_grams_id(const uint8_t id_type_len_bytes,
                                const TM_Gram_Id_Value_Ptr & one, const TM_Gram_Id_Value_Ptr & two) {
                            //First compare the types of the id
                            int result = compare(id_type_len_bytes, one, two);

                            //If the id types are equal now we need to compare the keys until the end
                            if (result == 0) {
                                //Extract the id type from the id
                                uint32_t type_one = 0;
                                copy_begin_bytes_to_end(one, id_type_len_bytes, type_one);

                                //Compute the length of the remainder of the m-gram id 
                                const uint8_t & id_len_bytes = gram_id_type_2_byte_len<CURR_LEVEL>(type_one);

                                //Compare the remainders of the m-gram ids
                                result = memcmp(one + id_type_len_bytes, two + id_type_len_bytes, id_len_bytes);
                            }

                            //The id types are not equal
                            return (result < 0);
                        }

                        /**
                         * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                         * @param id_len_bytes the minimum total number of bytes in both m-gram ids.
                         * @param one the first M-gram to compare
                         * @param two the second M-gram to compare
                         * @return true if the first M-gram is "larger" than the second, otherwise false
                         */
                        static inline bool is_more_m_grams_id(const uint8_t id_len_bytes,
                                const TM_Gram_Id_Value_Ptr & one, const TM_Gram_Id_Value_Ptr & two) {
                            return (compare(id_len_bytes, one, two) > 0);
                        };
                    };

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::NUM_BYTES_WORD_ID;

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::MAX_ID_LEN_BYTES[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::ID_TYPE_LEN_BYTES[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::NUMBER_ID_TYPES_PER_LEVEL[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_2_GRAM_TO_TYPE_LEN[NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_3_GRAM_TO_TYPE_LEN[NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_4_GRAM_TO_TYPE_LEN[NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_5_GRAM_TO_TYPE_LEN[NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint32_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_6_GRAM_TO_TYPE_LEN[NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID][NUM_BYTES_WORD_ID];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_2_GRAM_TO_BYTE_LEN[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_3_GRAM_TO_BYTE_LEN[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_4_GRAM_TO_BYTE_LEN[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_5_GRAM_TO_BYTE_LEN[];

                    template<typename TWordIdType, TModelLevel MAX_LEVEL>
                    constexpr uint8_t Byte_M_Gram_Id<TWordIdType, MAX_LEVEL>::LEVEL_6_GRAM_TO_BYTE_LEN[];

                }
            }
        }
    }
}



#endif	/* BYTEMGRAMID_HPP */

