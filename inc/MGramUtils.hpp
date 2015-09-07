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
                typedef uint8_t * M_Gram_Id;

                //The M-gram levels supported by the create function for M-gram id
                const static TModelLevel M_GRAM_LEVEL_2 = 2;
                const static TModelLevel M_GRAM_LEVEL_3 = 3;
                const static TModelLevel M_GRAM_LEVEL_4 = 4;
                const static TModelLevel M_GRAM_LEVEL_5 = 5;

                /**
                 * This method allows to get the number of bits needed to store this word id
                 * @param wordId the word id to analyze
                 * @return the number of bits needed to store this word id
                 */
                static inline uint8_t get_number_of_bits(const TShortId wordId) {
                    return log2_32(wordId);
                }

                
                template<uint8_t KEY_LEN_BITS, TModelLevel NUM_TOKENS>
                static inline bool create_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
                    //Declare and initialize the id length, the initial
                    //values is what we need to store the type
                    uint8_t id_len_bits = KEY_LEN_BITS;

                    //Obtain the word ids and their lengths in bits and
                    //the total length in bits needed to store the key
                    TShortId wordIds[NUM_TOKENS];
                    uint8_t len_bits[NUM_TOKENS];
                    for (size_t idx = (NUM_TOKENS - 1); idx >= 0; --idx) {
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
                    
                    //ToDo: Determine the size of id in bytes
                    
                    //ToDo: Allocate the id memory
                    
                    //ToDo: Determine the type id value from the bit lengths of the words
                    
                    //ToDo: Append the id type to the id
                    
                    //ToDo: Append the word id meaningful bits to the id in reverse order
                }
                
                /**
                 * This function allows to create an 2-gram id for a given 2-gram
                 * 
                 * 1) The 2 wordIds are to be converted to the 2-gram id:
                 * There are 32 bytes in one word id and 32 bytes in
                 * another word id, In total we have 32^2 possible 2-gram id
                 * lengths, if we only use meaningful bits of the word id for
                 * instance:
                 *       1-1 both really need just one bite
                 *       1-2 the first needs one and another two
                 *       2-1 the first needs two and another one
                 *       and so forth.
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
                static inline bool create_2_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
                    return create_gram_id<10, 2>(tokens, p_word_idx, m_gram_id);
                }

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
                static inline bool create_3_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
                    return create_gram_id<15, 3>(tokens, p_word_idx, m_gram_id);
                }

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
                static inline bool create_4_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
                    return create_gram_id<20, 4>(tokens, p_word_idx, m_gram_id);
                }

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
                static inline bool create_5_gram_id(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
                    return create_gram_id<25, 5>(tokens, p_word_idx, m_gram_id);
                }

                /**
                 * Define the function pointer to a create x-gram id function for some X-gram level x
                 */
                typedef bool(*create_x_gram_id)(const TextPieceReader *tokens, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id);

                //This is an array of functions for creating m-grams per specific m-gram level m
                const static create_x_gram_id create_x_gram_funcs[] = {create_2_gram_id, create_3_gram_id, create_4_gram_id, create_5_gram_id};

                /**
                 * This function allows to create an M-gram id for a given M-gram
                 * @param gram the M-gram to create the id for
                 * @param p_word_idx the used word index
                 * @param m_gram_id [out] the reference to the M-gram id to be created
                 * @return true if the M-gram id could be created, otherwise false
                 */
                static inline bool create_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, M_Gram_Id & m_gram_id) {
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
                }
            }
        }
    }
}

#endif	/* MGRAMUTILS_HPP */

