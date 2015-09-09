/* 
 * File:   MGram.hpp
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
 * Created on September 7, 2015, 9:42 PM
 */

#include "MGrams.hpp"

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
            namespace mgrams {
                namespace Comp_M_Gram_Id {

                    /**
                     * This method allows to get the number of bits needed to store this word id
                     * @param wordId the word id to analyze
                     * @return the number of bits needed to store this word id
                     */
                    static inline uint8_t get_number_of_bits(const uint32_t wordId) {
                        return log2_32(wordId) + 1;
                    };

                    /**
                     * Stores the multipliers up to and including level 6
                     */
                    const static uint32_t gram_id_type_mult[] = {1, 32, 32 * 32, 32 * 32 * 32, 32 * 32 * 32 * 32, 32 * 32 * 32 * 32 * 32};

                    /**
                     * This method is needed to compute the id type identifier.
                     * Can compute the id type for M-grams until (including) M = 6
                     * The type is computed as in a 32-based numeric system, e.g. for M==5:
                     *          (len_bits[0]-1)*32^0 + (len_bits[1]-1)*32^1 +
                     *          (len_bits[2]-1)*32^2 + (len_bits[3]-1)*32^3 +
                     *          (len_bits[4]-1)*32^4
                     * @param the number of word ids
                     * @param len_bits the bits needed per word id
                     * @param id_type [out] the resulting id type the initial value is expected to be 0
                     */
                    template<TModelLevel M_GRAM_LEVEL>
                    void get_gram_id_type(uint8_t len_bits[M_GRAM_LEVEL], uint32_t & id_type) {
                        //Do the sanity check for against overflows
                        if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                            stringstream msg;
                            msg << "get_gram_id_type: Unsupported m-gram level: "
                                    << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_6) << "], insufficient multipliers!";
                            throw Exception(msg.str());
                        }

                        LOG_DEBUG2 << "Computing the " << SSTR(M_GRAM_LEVEL) << "-gram id type" << END_LOG;

                        //Compute the M-gram id type. Here we use the pre-computed multipliers
                        for (size_t idx = 0; idx < M_GRAM_LEVEL; ++idx) {
                            LOG_DEBUG3 << "len_bits[" << SSTR(idx) << "] = " << SSTR((uint32_t) len_bits[idx]) << END_LOG;
                            id_type += ((uint32_t) len_bits[idx] - 1) * gram_id_type_mult[idx];
                        }
                        LOG_DEBUG3 << "Resulting id_type = " << SSTR(id_type) << END_LOG;
                    };

                    /**
                     * Allows to extract the M-gram id length from the given M-gram level M and id
                     * This implementation should work up to 6-grams! If we use it for
                     * 7-grams then the internal computations will overflow!
                     * @param ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                     * @param M_GRAM_LEVEL the number of tokens in the M-gram
                     * @param m_gram_id the given M-gram id
                     * @param len_bytes [out] the M-gram id length in bytes
                     */
                    template<uint8_t ID_TYPE_LEN_BITS, TModelLevel M_GRAM_LEVEL >
                    void get_gram_id_len(const uint8_t * m_gram_id, uint8_t & len_bytes) {
                        //Declare and initialize the id length, the initial values is
                        //what we need to store the type. Note that, the maximum number
                        //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                        //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                        //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                        //This is when we get an overflow here if we use uint8_t to
                        //store the length in bits will overflow. Therefore the sanity check.
                        uint8_t id_len_bits = ID_TYPE_LEN_BITS;

                        //If needed, do the sanity checks
                        if (DO_SANITY_CHECKS) {
                            if (m_gram_id == NULL) {
                                throw Exception("get_m_gram_id_length: A NULL pointer M-gram id!");
                            }
                            if (M_GRAM_LEVEL > M_GRAM_LEVEL_6) {
                                stringstream msg;
                                msg << "get_gram_id_len: Unsupported m-gram level: "
                                        << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                        << SSTR(M_GRAM_LEVEL_2) << ", "
                                        << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                                throw Exception(msg.str());
                            }
                        }

                        //1. ToDo: Extract the id_type from the M-gram
                        uint32_t id_type = 0;
                        copy_begin_bits_to_end<ID_TYPE_LEN_BITS>(m_gram_id, id_type);

                        //2. Compute the M-gram id length from the M-gram id type.
                        //   Here we use the pre-computed multipliers we add the
                        //   final bits at the end of the function.
                        uint8_t coeff = 0;
                        for (int idx = (M_GRAM_LEVEL - 1); idx >= 0; --idx) {
                            //"coeff = len_bits[idx] - 1"
                            coeff = (uint8_t) (id_type / gram_id_type_mult[idx]);
                            LOG_DEBUG3 << SSTR(id_type) << " / " << SSTR(gram_id_type_mult[idx]) << " =  " << SSTR((uint32_t) coeff) << END_LOG;
                            id_type -= coeff * gram_id_type_mult[idx];
                            id_len_bits += coeff;
                        }

                        LOG_DEBUG3 << "id_len_bits: " << SSTR((uint32_t) id_len_bits) << END_LOG;
                        LOG_DEBUG3 << "id_len_bits: " << SSTR((uint32_t) M_GRAM_LEVEL) << END_LOG;

                        //Note that in the loop above we have "coeff = len_bits[idx] - 1"
                        //Therefore, here we add the number of tokens to account for this -1's
                        len_bytes = NUM_BITS_TO_STORE_BYTES(id_len_bits + (uint8_t) M_GRAM_LEVEL);
                    };

                    /**
                     * This method is needed to compute the M-gram id.
                     * 
                     * This implementation should work up to 6-grams! If we use it for
                     * 7-grams then the internal computations will overflow!
                     * 
                     * Let us give an example of a 2-gram id for a given 2-gram:
                     * 
                     * 1) The 2 wordIds are to be converted to the 2-gram id:
                     * There are 32 bytes in one word id and 32 bytes in
                     * another word id, In total we have 32^2 possible 2-gram id
                     * lengths, if we only use meaningful bits of the word id for
                     * instance:
                     *       01-01 both really need just one bite
                     *       01-02 the first needs one and another two
                     *       02-01 the first needs two and another one
                     *       ...
                     *       32-32 both need 32 bits
                     * 
                     * 2) These 32^2 = 1,024 combinations uniquely identify the
                     * type of stored id. So this can be an uid of the gram id type.
                     * To store such a uid we need log2(1,024)= 10 bits.
                     * 
                     * 3) We create the 2-gram id as a byte array of 10 bits - type
                     * + the meaningful bits from wordId2 and wordId1. We start from
                     * the end (reverse the word order) as this can potentially
                     * increase speed of the comparison operation.
                     * 
                     * 4) The total length of the 2-gram id in bytes is the number
                     * of bits needed to store the key type and meaningful word id
                     * bits, rounded up to the full bytes.
                     * 
                     * @param ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                     * @param M_GRAM_LEVEL the number of tokens in the M-gram
                     * @param tokens the M-gram tokens
                     * @param p_word_idx the word index
                     * @param m_p_p_gram_id the pointer to the pointer to store the M-gram id data pointer.
                     * If (*m_p_p_gram_id == NULL) new memory will be allocated for the id, otherwise the
                     * pointer will be used to store the id. It is then assumed that there is enough memory
                     * allocated to store the id. For an M-gram it is (4*M) bytes that is needed to store
                     * the longed M-gram id.
                     * @return true if the m-gram id could be computed, otherwise false
                     */
                    template<uint8_t ID_TYPE_LEN_BITS, TModelLevel M_GRAM_LEVEL>
                    static inline bool create_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        //Declare and initialize the id length, the initial values is
                        //what we need to store the type. Note that, the maximum number
                        //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                        //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                        //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                        //This is when we get an overflow here if we use uint8_t to
                        //store the length in bits will overflow. Therefore the sanity check.
                        uint8_t id_len_bits = ID_TYPE_LEN_BITS;

                        LOG_DEBUG2 << "Creating the " << SSTR(M_GRAM_LEVEL) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) ID_TYPE_LEN_BITS) << END_LOG;

                        //Do the sanity check for against overflows
                        if (DO_SANITY_CHECKS && (M_GRAM_LEVEL > M_GRAM_LEVEL_6)) {
                            stringstream msg;
                            msg << "create_gram_id: Unsupported m-gram level: "
                                    << SSTR(M_GRAM_LEVEL) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                            throw Exception(msg.str());
                        }

                        //Obtain the word ids and their lengths in bits and
                        //the total length in bits needed to store the key
                        TShortId wordIds[M_GRAM_LEVEL];
                        uint8_t len_bits[M_GRAM_LEVEL];
                        for (size_t idx = 0; idx < M_GRAM_LEVEL; ++idx) {
                            //Get the word id if possible
                            if (p_word_idx->get_word_id(tokens[idx].str(), wordIds[idx])) {
                                //Get the number of bits needed to store this id
                                len_bits[idx] = get_number_of_bits(wordIds[idx]);
                                LOG_DEBUG3 << "Word [ " << tokens[idx].str() << " ] id: "
                                        << SSTR(wordIds[idx]) << " len. in bits: "
                                        << SSTR((uint32_t) len_bits[idx]) << END_LOG;
                                //Compute the total gram id length in bits
                                id_len_bits += len_bits[idx];
                            } else {
                                //The word id could not be found so there is no need to proceed
                                return false;
                            }
                        }
                        LOG_DEBUG3 << "Total len. in bits: " << SSTR((uint32_t) id_len_bits) << END_LOG;

                        //Determine the size of id in bytes, divide with rounding up
                        const uint8_t id_len_bytes = NUM_BITS_TO_STORE_BYTES(id_len_bits);

                        LOG_DEBUG3 << "Total len. in bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                        //Allocate the id memory if there was nothing pre-allocated yet
                        if (m_p_gram_id == NULL) {
                            //Allocate memory
                            m_p_gram_id = new uint8_t[id_len_bytes];
                            LOG_DEBUG3 << "Created a Compressed_M_Gram_Id: " << SSTR((void *) m_p_gram_id) << END_LOG;
                        }
                        //Clean the memory
                        memset(m_p_gram_id, 0, id_len_bytes);

                        //Determine the type id value from the bit lengths of the words
                        uint32_t id_type_value = 0;
                        get_gram_id_type<M_GRAM_LEVEL>(len_bits, id_type_value);

                        //Append the id type to the M-gram id
                        uint8_t to_bit_pos = 0;
                        copy_end_bits_to_pos(id_type_value, ID_TYPE_LEN_BITS, m_p_gram_id, to_bit_pos);
                        to_bit_pos += ID_TYPE_LEN_BITS;

                        //Append the word id meaningful bits to the id in reverse order
                        for (int idx = (M_GRAM_LEVEL - 1); idx >= 0; --idx) {
                            copy_end_bits_to_pos(wordIds[idx], len_bits[idx], m_p_gram_id, to_bit_pos);
                            to_bit_pos += len_bits[idx];
                        }

                        LOG_DEBUG2 << "Finished making the " << SSTR(M_GRAM_LEVEL) << "-gram id with "
                                << "id type length: " << SSTR((uint32_t) ID_TYPE_LEN_BITS) << END_LOG;

                        return true;
                    };

                    static inline bool create_2_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        return create_gram_id < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL_2 - M_GRAM_LEVEL_2], M_GRAM_LEVEL_2 >
                                (tokens, p_word_idx, m_p_gram_id);
                    }

                    static inline bool create_3_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        return create_gram_id < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL_3 - M_GRAM_LEVEL_2], M_GRAM_LEVEL_3 >
                                (tokens, p_word_idx, m_p_gram_id);
                    }

                    static inline bool create_4_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        return create_gram_id < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL_4 - M_GRAM_LEVEL_2], M_GRAM_LEVEL_4 >
                                (tokens, p_word_idx, m_p_gram_id);
                    }

                    static inline bool create_5_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        return create_gram_id < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL_5 - M_GRAM_LEVEL_2], M_GRAM_LEVEL_5 >
                                (tokens, p_word_idx, m_p_gram_id);
                    }

                    /**
                     * Define the function pointer to a create x-gram id function for some X-gram level x
                     */
                    typedef bool(*create_x_gram_id)(const TextPieceReader *tokens, const AWordIndex * p_word_idx,
                            T_Comp_M_Gram_Id_Ptr & m_p_gram_id);

                    //This is an array of functions for creating m-grams per specific m-gram level m
                    const static create_x_gram_id create_x_gram_funcs[] = {create_2_gram_id, create_3_gram_id, create_4_gram_id, create_5_gram_id};

                    bool create_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        if (DO_SANITY_CHECKS && ((gram.level < M_GRAM_LEVEL_2) || (gram.level > M_GRAM_LEVEL_5))) {
                            stringstream msg;
                            msg << "create_m_gram_id: Unsupported m-gram level: "
                                    << SSTR(gram.level) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_5) << "]";
                            throw Exception(msg.str());
                        }

                        LOG_DEBUG2 << "Creating the " << SSTR(gram.level) << "-gram id for: " << tokensToString(gram) << END_LOG;

                        //Call the appropriate function, use array instead of switch, should be faster.
                        return create_x_gram_funcs[gram.level - M_GRAM_LEVEL_2](gram.tokens, p_word_idx, m_p_gram_id);
                    }

                    template<bool IS_LESS, TModelLevel M_GRAM_LEVEL>
                    bool compare(const T_Comp_M_Gram_Id_Ptr & m_p_gram_id_one, const T_Comp_M_Gram_Id_Ptr & m_p_gram_id_two) {
                        //Get the id len in bits
                        constexpr uint8_t ID_TYPE_LEN_BITS = M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL - M_GRAM_LEVEL_2];

                        //Get the M-gram type ids
                        TShortId type_one = 0;
                        copy_begin_bits_to_end < ID_TYPE_LEN_BITS >(m_p_gram_id_one, type_one);
                        TShortId type_two = 0;
                        copy_begin_bits_to_end < ID_TYPE_LEN_BITS >(m_p_gram_id_two, type_two);

                        if (type_one < type_two) {
                            //The first id type is smaller
                            LOG_DEBUG3 << SSTR((void*) m_p_gram_id_one) << (IS_LESS ? "<" : ">") << SSTR((void*) m_p_gram_id_two) << END_LOG;
                            return IS_LESS;
                        } else {
                            if (type_one > type_two) {
                                //The second id type is smaller
                                LOG_DEBUG3 << SSTR((void*) m_p_gram_id_one) << (!IS_LESS ? "<" : ">") << SSTR((void*) m_p_gram_id_two) << END_LOG;
                                return !IS_LESS;
                            } else {
                                //The id types are the same! Compare the ids themselves

                                //Get one of the lengths, as they both are the same
                                uint8_t id_len_bytes = 0;
                                get_gram_id_len < M_GRAM_ID_TYPE_LEN_BITS[M_GRAM_LEVEL - M_GRAM_LEVEL_2], M_GRAM_LEVEL > (m_p_gram_id_one, id_len_bytes);

                                LOG_DEBUG3 << "ID_TYPE_LEN_BITS: " << SSTR((uint32_t) ID_TYPE_LEN_BITS)
                                        << ", idx: " << SSTR((uint32_t) NUM_FULL_BYTES(ID_TYPE_LEN_BITS))
                                        << ", id_len_bytes: " << SSTR((uint32_t) id_len_bytes) << END_LOG;

                                //Start comparing the ids byte by byte but not from the fist
                                //bytes as this is where the id type information is stored,
                                //start with the first byte that contains key information
                                for (uint8_t idx = NUM_FULL_BYTES(ID_TYPE_LEN_BITS); idx < id_len_bytes; ++idx) {
                                    if (m_p_gram_id_one[idx] < m_p_gram_id_two[idx]) {
                                        LOG_DEBUG3 << SSTR((void*) m_p_gram_id_one) << (IS_LESS ? "<" : ">") << SSTR((void*) m_p_gram_id_two) << END_LOG;
                                        return IS_LESS;
                                    } else {
                                        if (m_p_gram_id_one[idx] > m_p_gram_id_two[idx]) {
                                            LOG_DEBUG3 << SSTR((void*) m_p_gram_id_one) << (!IS_LESS ? "<" : ">") << SSTR((void*) m_p_gram_id_two) << END_LOG;
                                            return !IS_LESS;
                                        } else {
                                            //Nothing to return yet, so far the values are equal, keep iterating
                                        }
                                    }
                                }

                                //We've finished iterating and since we are still here, the ids are equal
                                LOG_DEBUG3 << SSTR((void*) m_p_gram_id_one) << " = " << SSTR((void*) m_p_gram_id_two) << END_LOG;
                                return !IS_LESS;
                            }
                        }
                    };

                    /***********************************************************************************************************************/

                    /**
                     * Define the function pointer to compare two X-grams of the given level X
                     */
                    typedef bool(* is_compare_grams_id_func)(const T_Comp_M_Gram_Id_Ptr &, const T_Comp_M_Gram_Id_Ptr &);

                    static inline bool is_less_2_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<true, M_GRAM_LEVEL_2>(one, two);
                    }

                    static inline bool is_less_3_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<true, M_GRAM_LEVEL_3>(one, two);
                    }

                    static inline bool is_less_4_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<true, M_GRAM_LEVEL_4>(one, two);
                    }

                    static inline bool is_less_5_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<true, M_GRAM_LEVEL_5>(one, two);
                    }

                    //This is an array of functions for comparing x-grams of level x
                    const static is_compare_grams_id_func is_less_x_grams_id_funcs[] = {is_less_2_grams_id, is_less_3_grams_id, is_less_4_grams_id, is_less_5_grams_id};

                    bool is_less_m_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two, const TModelLevel level) {
                        return is_less_x_grams_id_funcs[level - M_GRAM_LEVEL_2](one, two);
                    }

                    static inline bool is_more_2_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<false, M_GRAM_LEVEL_2>(one, two);
                    }

                    static inline bool is_more_3_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<false, M_GRAM_LEVEL_3>(one, two);
                    }

                    static inline bool is_more_4_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<false, M_GRAM_LEVEL_4>(one, two);
                    }

                    static inline bool is_more_5_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two) {
                        return compare<false, M_GRAM_LEVEL_5>(one, two);
                    }

                    //This is an array of functions for comparing x-grams of level x
                    const static is_compare_grams_id_func is_more_x_grams_id_funcs[] = {is_more_2_grams_id, is_more_3_grams_id, is_more_4_grams_id, is_more_5_grams_id};

                    bool is_more_m_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two, const TModelLevel level) {
                        return is_more_x_grams_id_funcs[level - M_GRAM_LEVEL_2](one, two);
                    }
                }
            }
        }
    }
}