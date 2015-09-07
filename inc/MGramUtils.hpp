/* 
 * File:   MGramUtils.hpp
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
 * Created on September 7, 2015, 10:35 AM
 */

#ifndef MGRAMUTILS_HPP
#define	MGRAMUTILS_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

using namespace std;
using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {
            namespace utils {

                /**
                 * This structure is used to store the N-Gram data
                 * of the back-off Language Model.
                 * @param prob stores the log_10 probability of the N-Gram Must be
                 *             a negative value
                 * @param back_off stores the log_10 back-off weight (probability)
                 *        of the N-gram can be 0 is the probability is not available
                 * @param context stores the n-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
                 * @param tokens stores the N-gram words the size of this vector
                 *        defines the N-gram level.
                 * @param level stores the number of meaningful elements in the tokens, the value of N for the N-gram
                 */
                typedef struct {
                    TLogProbBackOff prob;
                    TLogProbBackOff back_off;
                    TextPieceReader context;
                    TextPieceReader tokens[MAX_NGRAM_LEVEL];
                    TModelLevel level;

                    /**
                     * This function allows to compute the hash of the given M-Gram
                     * It assumes, which should hold, that the memory pointed by the tokens is continuous
                     * @return the hash value of the given token
                     */
                    inline TShortId hash() const {
                        //Compute the length of the gram tokens in memory, including spaces between
                        const char * beginFirstPtr = tokens[0].getBeginCStr();
                        const TextPieceReader & last = tokens[level - 1];
                        const char * beginLastPtr = last.getBeginCStr();
                        const size_t totalLen = (beginLastPtr - beginFirstPtr) + last.getLen();

                        //If the sanity check is on then test that the memory is continuous
                        //Compute the same length but with a longer iterative algorithms
                        if (DO_SANITY_CHECKS) {
                            //Compute the exact length
                            size_t exactTotalLen = level - 1; //The number of spaces in between tokens
                            for (TModelLevel idx = 0; idx < level; idx++) {
                                exactTotalLen += tokens[idx].getLen();
                            }
                            //Check that the exact and fast computed lengths are the same
                            if (exactTotalLen != totalLen) {
                                stringstream msg;
                                msg << "The memory allocation for M-gram tokens is not continuous: totalLen (" <<
                                        SSTR(totalLen) << ") != exactTotalLen (" << SSTR(exactTotalLen) << ")";
                                throw Exception(msg.str());
                            }
                        }

                        //Compute the hash using the gram tokens with spaces with them
                        return computePaulHsiehHash(beginFirstPtr, totalLen);
                    }
                } T_M_Gram;

                //The M-gram Id will be an array of bytes which will be of a variable length
                typedef uint8_t * T_M_Gram_Id;

                //The M-gram levels supported by the create function for M-gram id
                const static TModelLevel M_GRAM_LEVEL_2 = 2;
                const static TModelLevel M_GRAM_LEVEL_3 = 3;
                const static TModelLevel M_GRAM_LEVEL_4 = 4;
                const static TModelLevel M_GRAM_LEVEL_5 = 5;
                const static TModelLevel M_GRAM_LEVEL_6 = 6;
                const static TModelLevel M_GRAM_LEVEL_7 = 7;

                //The memory in bits needed to store different M-gram id types in the M-gram id byte arrays
                static const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS = 10;
                static const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS = 15;
                static const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS = 20;
                static const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS = 25;

                /**
                 * This method allows to get the number of bits needed to store this word id
                 * @param wordId the word id to analyze
                 * @return the number of bits needed to store this word id
                 */
                static inline uint8_t get_number_of_bits(const TShortId wordId) {
                    return log2_32(wordId);
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
                template<TModelLevel NUM_TOKENS>
                void get_gram_id_type(uint8_t len_bits[NUM_TOKENS], TShortId & id_type) {
                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (NUM_TOKENS > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_gram_id_type: Unsupported m-gram level: "
                                << SSTR(NUM_TOKENS) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], insufficient multipliers!";
                        throw Exception(msg.str());
                    }

                    //Compute the M-gram id type. Here we use the pre-computed multipliers
                    for (size_t idx = 0; idx < NUM_TOKENS; ++idx) {
                        id_type += (len_bits[idx] - 1) * gram_id_type_mult[idx];
                    }
                };

                /**
                 * Allows to delete the allocated M-Gram id. If the id is NULL nothing is done.
                 * If the id is not null then its memory is freed and the id is set to NULL.
                 * @param m_gram_id the m-gram to delete (free memory for)
                 */
                static inline void delete_gram_id(T_M_Gram_Id & m_gram_id) {
                    if (m_gram_id != NULL) {
                        delete[] m_gram_id;
                        m_gram_id = NULL;
                    }
                };

                /**
                 * This method is needed to compute the M-gram id.
                 * This implementation should work up to 6-grams! If we use it for
                 * 7-grams then the internal computations will overflow!
                 * @param MAX_ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                 * @param NUM_TOKENS the number of tokens in the M-gram
                 * @param tokens the M-gram tokens
                 * @param p_word_idx the word index
                 * @param m_gram_id the resulting m-gram id, if NULL new memory
                 * will be allocated for the id, otherwise the pointer will be
                 * used to store the id. It is then assumed that there is enough
                 * memory allocated to store the id. For an M-gram it is (4*M)
                 * bytes that is needed to store the longed M-gram id.
                 * @return true if the m-gram id could be computed, otherwise false
                 */
                template<uint8_t MAX_ID_TYPE_LEN_BITS, TModelLevel NUM_TOKENS>
                static inline bool create_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    //Declare and initialize the id length, the initial values is
                    //what we need to store the type. Note that, the maximum number
                    //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                    //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                    //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                    //This is when we get an overflow here if we use uint8_t to
                    //store the length in bits will overflow. Therefore the sanity check.
                    uint8_t id_len_bits = MAX_ID_TYPE_LEN_BITS;

                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (NUM_TOKENS > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "create_gram_id: Unsupported m-gram level: "
                                << SSTR(NUM_TOKENS) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    //Obtain the word ids and their lengths in bits and
                    //the total length in bits needed to store the key
                    TShortId wordIds[NUM_TOKENS];
                    uint8_t len_bits[NUM_TOKENS];
                    for (size_t idx = 0; idx < NUM_TOKENS; ++idx) {
                        //Get the word id if possible
                        if (p_word_idx->get_word_id(tokens[idx].str(), wordIds[idx])) {
                            //Get the number of bits needed to store this id
                            len_bits[idx] = get_number_of_bits(wordIds[idx]);
                            //Compute the total gram id length in bits
                            id_len_bits += len_bits[idx];
                        } else {
                            //The word id could not be found so there is no need to proceed
                            return false;
                        }
                    }
                    //Determine the size of id in bytes, divide with rounding up
                    const uint8_t id_len_bytes = NUM_BITS_TO_STORE_BYTES(id_len_bits);

                    //Allocate the id memory if there was nothing pre-allocated yet
                    if (m_gram_id == NULL) {
                        //Allocate memory
                        m_gram_id = new uint8_t[id_len_bytes];
                    } else {
                        //Clean the memory
                        memset(m_gram_id, 0, id_len_bytes);
                    }

                    //Determine the type id value from the bit lengths of the words
                    TShortId id_type_value = 0;
                    get_gram_id_type<NUM_TOKENS>(len_bits, id_type_value);

                    //Append the id type to the M-gram id
                    uint8_t to_bit_pos = 0;
                    copy_end_bits(id_type_value, MAX_ID_TYPE_LEN_BITS, m_gram_id, to_bit_pos);
                    to_bit_pos += MAX_ID_TYPE_LEN_BITS;

                    //Append the word id meaningful bits to the id in reverse order
                    for (size_t idx = (NUM_TOKENS - 1); idx >= 0; --idx) {
                        copy_end_bits(wordIds[idx], len_bits[idx], m_gram_id, to_bit_pos);
                        to_bit_pos += len_bits[idx];
                    }

                    return true;
                };

                /**
                 * This function allows to create an 2-gram id for a given 2-gram
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
                 * @param tokens the M-gram tokens to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_2_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    return create_gram_id<M_GRAM_2_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_2>(tokens, p_word_idx, m_gram_id);
                };

                /**
                 * This function allows to create an 3-gram id for a given 3-gram
                 * Works by the same principle as create_2_gram_id.
                 * Possible id types: 32^3 = 32,768
                 * The number of bits needed to store the type is log_2(32,768) = 15
                 * @param tokens the M-gram tokens to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_3_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    return create_gram_id<M_GRAM_3_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_3>(tokens, p_word_idx, m_gram_id);
                };

                /**
                 * This function allows to create an 4-gram id for a given 4-gram
                 * Works by the same principle as create_2_gram_id.
                 * Possible id types: 32^4 = 1,048,576
                 * The number of bits needed to store the type is log_2(1,048,576) = 20
                 * @param tokens the M-gram tokens to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_4_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    return create_gram_id<M_GRAM_4_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_4>(tokens, p_word_idx, m_gram_id);
                };

                /**
                 * This function allows to create an 5-gram id for a given 5-gram
                 * Works by the same principle as create_2_gram_id.
                 * Possible id types: 32^5 = 33,554,432
                 * The number of bits needed to store the type is log_2(33,554,432) = 25
                 * @param tokens the M-gram tokens to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_5_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    return create_gram_id<M_GRAM_5_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_5>(tokens, p_word_idx, m_gram_id);
                };

                /**
                 * Define the function pointer to a create x-gram id function for some X-gram level x
                 */
                typedef bool(*create_x_gram_id)(const TextPieceReader *tokens, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id);

                //This is an array of functions for creating m-grams per specific m-gram level m
                const static create_x_gram_id create_x_gram_funcs[] = {create_2_gram_id, create_3_gram_id, create_4_gram_id, create_5_gram_id};

                /**
                 * This function allows to create an M-gram id for a given M-gram
                 * @param gram the M-gram to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, T_M_Gram_Id & m_gram_id) {
                    if (DO_SANITY_CHECKS && ((gram.level < M_GRAM_LEVEL_2) || (gram.level > M_GRAM_LEVEL_5))) {
                        stringstream msg;
                        msg << "create_m_gram_id: Unsupported m-gram level: "
                                << SSTR(gram.level) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_5) << "]";
                        throw Exception(msg.str());
                    }

                    //Call the appropriate function, use array instead of switch, should be faster.
                    return create_x_gram_funcs[gram.level - M_GRAM_LEVEL_2](gram.tokens, p_word_idx, m_gram_id);
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * This implementation should work up to 6-grams! If we use it for
                 * 7-grams then the internal computations will overflow!
                 * @param MAX_ID_TYPE_LEN_BITS the maximum number of bites needed to store the id type in bits
                 * @param NUM_TOKENS the number of tokens in the M-gram
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                template<uint8_t MAX_ID_TYPE_LEN_BITS, TModelLevel NUM_TOKENS >
                uint8_t get_gram_id_len(const T_M_Gram_Id & m_gram_id) {
                    //Declare and initialize the id length, the initial values is
                    //what we need to store the type. Note that, the maximum number
                    //of needed bits for an id for a 5-gram is 25+32+32+32+32+32 = 185
                    //bits. For a 6-gram we get 30+32+32+32+32+32+32 = 222 bits,
                    //for a 7-gram we get 35+32+32+32+32+32+32+32 = 259 bits!
                    //This is when we get an overflow here if we use uint8_t to
                    //store the length in bits will overflow. Therefore the sanity check.
                    uint8_t id_len_bits = MAX_ID_TYPE_LEN_BITS;

                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (NUM_TOKENS > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_gram_id_len: Unsupported m-gram level: "
                                << SSTR(NUM_TOKENS) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    //1. ToDo: Extract the id_type from the M-gram
                    TShortId id_type;
                    copy_begin_bits_to_end<MAX_ID_TYPE_LEN_BITS>(m_gram_id, id_type);

                    //2. Compute the M-gram id length from the M-gram id type.
                    //   Here we use the pre-computed multipliers we add the
                    //   final bits at the end of the function.
                    uint8_t coeff;
                    for (size_t idx = (NUM_TOKENS - 1); idx >= 0; --idx) {

                        coeff = id_type / gram_id_type_mult[idx];
                        id_type -= coeff * gram_id_type_mult[idx];
                        id_len_bits += coeff;
                    }

                    //Note that in the loop above we have "coeff = len_bits[idx] - 1"
                    //Therefore, here we add the number of tokens to account for this -1's
                    return NUM_BITS_TO_STORE_BYTES(id_len_bits + NUM_TOKENS);
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                static inline uint8_t get_2_gram_id_len(const T_M_Gram_Id & m_gram_id) {

                    return get_gram_id_len<M_GRAM_2_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_2>(m_gram_id);
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                static inline uint8_t get_3_gram_id_len(const T_M_Gram_Id & m_gram_id) {

                    return get_gram_id_len<M_GRAM_3_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_3>(m_gram_id);
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                static inline uint8_t get_4_gram_id_len(const T_M_Gram_Id & m_gram_id) {

                    return get_gram_id_len<M_GRAM_4_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_4>(m_gram_id);
                };

                /**
                 * Allows to extract the M-gram id length from the given M-gram level M and id
                 * @param m_gram_id the given M-gram id
                 * @return the M-gram id length in bytes
                 */
                static inline uint8_t get_5_gram_id_len(const T_M_Gram_Id & m_gram_id) {

                    return get_gram_id_len<M_GRAM_5_ID_TYPE_LEN_BITS, M_GRAM_LEVEL_5>(m_gram_id);
                };

                /**
                 * Define the function pointer to a get x-gram id length for some X-gram level x
                 */
                typedef uint8_t(*get_x_gram_id_len)(const T_M_Gram_Id & m_gram_id);

                //This is an array of functions for creating m-grams per specific m-gram level m
                const static get_x_gram_id_len get_x_gram_id_len_funcs[] = {get_2_gram_id_len, get_3_gram_id_len, get_4_gram_id_len, get_5_gram_id_len};

                /**
                 * Allows to extract the M-gram id length in bytes
                 * Note that this method is applicable only for M-grams with M <= 6;
                 * @param m_gram_id the M-gram id to extract the length for
                 * @param level the M-gram level
                 * @return the M-gram id length in bytes
                 */
                static inline uint8_t get_m_gram_id_len(const T_M_Gram_Id & m_gram_id, const TModelLevel level) {
                    //Do the sanity check for against overflows
                    if (DO_SANITY_CHECKS && (level > M_GRAM_LEVEL_6)) {
                        stringstream msg;
                        msg << "get_m_gram_id_len: Unsupported m-gram level: "
                                << SSTR(level) << ", must be within ["
                                << SSTR(M_GRAM_LEVEL_2) << ", "
                                << SSTR(M_GRAM_LEVEL_6) << "], otherwise overflows!";
                        throw Exception(msg.str());
                    }

                    if (m_gram_id != NULL) {
                        //Call the appropriate function, use array instead of switch should be faster.
                        return get_x_gram_id_len_funcs[level - M_GRAM_LEVEL_2](m_gram_id);
                    } else {
                        throw Exception("get_m_gram_id_length: A NULL pointer M-gram id!");
                    }
                };

                /**
                 * Allows to compare two M-Gram ids depending on the template flag it is a different operator
                 * @param IS_LESS if true the it is a is_less compare, if false then is_more
                 * @param one the first M-gram id
                 * @param two the second M-gram id
                 * @return true if "one < two" otherwise false
                 */
                template<bool IS_LESS>
                static inline bool compare(const T_M_Gram_Id & one, const T_M_Gram_Id & two, const TModelLevel level) {
                    //Get the M-gram type ids
                    TShortId type_one;
                    //ToDo: Implement
                    TShortId type_two;
                    //ToDo: Implement

                    if (type_one < type_two) {
                        //The first id type is smaller
                        return IS_LESS;
                    } else {
                        if (type_one > type_two) {
                            //The second id type is smaller
                            return !IS_LESS;
                        } else {
                            //The id types are the same! Compare the ids themselves

                            //Get one of the lengths
                            const uint8_t len = get_m_gram_id_len(one, level);

                            //Start comparing the ids byte by byte but not from the fist
                            //bytes as this is where the id type information is stored 

                            //ToDo: Implement
                        }
                    }
                };
            }
        }
    }
}

#endif	/* MGRAMUTILS_HPP */

