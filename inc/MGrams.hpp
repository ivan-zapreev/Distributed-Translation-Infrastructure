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

#ifndef MGRAM_HPP
#define	MGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

using namespace std;
using namespace uva::utils::math::log2;
using namespace uva::utils::math::bits;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {
            namespace mgrams {

                //Various M-gram levels
                const static TModelLevel M_GRAM_LEVEL_UNDEF = 0u;
                const static TModelLevel M_GRAM_LEVEL_1 = 1u;
                const static TModelLevel M_GRAM_LEVEL_2 = 2u;
                const static TModelLevel M_GRAM_LEVEL_3 = 3u;
                const static TModelLevel M_GRAM_LEVEL_4 = 4u;
                const static TModelLevel M_GRAM_LEVEL_5 = 5u;
                const static TModelLevel M_GRAM_LEVEL_6 = 6u;
                const static TModelLevel M_GRAM_LEVEL_7 = 7u;

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
                    TextPieceReader tokens[M_GRAM_LEVEL_MAX];
                    TModelLevel level;

                    /**
                     * This function allows to compute the hash of the sub M-Gram
                     * starting from and including the word on the given index,
                     * and until and including the word of the given index. It
                     * assumes, which should hold, that the memory pointed by the
                     * tokens is continuous.
                     * @param begin_idx  the index of the first word in tokens array
                     * @param end_idx the index of the last word in tokens array
                     * @return the hash value of the given token
                     */
                    inline uint64_t sub_hash(const TModelLevel begin_idx, const TModelLevel end_idx) const {
                        LOG_DEBUG3 << "Hashing tokens begin_idx: " << begin_idx << ", end_idx: " << end_idx << END_LOG;

                        //Compute the length of the gram tokens in memory, including spaces between
                        const char * beginFirstPtr = tokens[begin_idx].getBeginCStr();
                        const TextPieceReader & last = tokens[end_idx];
                        const char * beginLastPtr = last.getBeginCStr();
                        const size_t totalLen = (beginLastPtr - beginFirstPtr) + last.getLen();
                        LOG_DEBUG3 << "Hashing tokens length: " << totalLen << END_LOG;

                        //If the sanity check is on then test that the memory is continuous
                        //Compute the same length but with a longer iterative algorithms
                        if (DO_SANITY_CHECKS) {
                            //Compute the exact length
                            size_t exactTotalLen = (end_idx - begin_idx); //The number of spaces in between tokens
                            for (TModelLevel idx = begin_idx; idx <= end_idx; idx++) {
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
#ifdef ENVIRONMENT64
                        return XXH64(beginFirstPtr, totalLen, XXHASH_SEED);
#else
                        return XXH32(beginFirstPtr, totalLen, XXHASH_SEED);
#endif
                    }

                    /**
                     * This function allows to compute the hash of the M-Gram suffix
                     * starting from and including the word on the given index. It
                     * assumes, which should hold, that the memory pointed by the
                     * tokens is continuous.
                     * @param begin_idx  the index of the first word in tokens array
                     * @return the hash value of the given token
                     */
                    inline uint64_t suffix_hash(const TModelLevel begin_idx) const {
                        const TModelLevel end_idx = level - 1;
                        return sub_hash(begin_idx, end_idx);
                    }

                    /**
                     * This function allows to compute the hash of the given M-Gram
                     * It assumes, which should hold, that the memory pointed by the tokens is continuous
                     * @return the hash value of the given token
                     */
                    inline uint64_t hash() const {
                        return suffix_hash(0);
                    }
                } T_M_Gram;

                /**
                 * This function allows to convert the M-gram tokens into a string representation. 
                 * @param gram  the M-Gram to work with
                 */
                template<TModelLevel N = M_GRAM_LEVEL_MAX>
                inline string tokensToString(const T_M_Gram & gram) {
                    return tokensToString<N>(gram.tokens, gram.level);
                };

                //The memory in bits needed to store different M-gram id types in
                //the M-gram id byte arrays
                //ToDo: These are the minimum values, if used then we use fewest memory
                //but the data is not byte aligned. Therefore some bit copying operations
                //are not done efficiently. Perhaps it is worth trying to round these
                //values up to full bytes, this can improve performance @ some memory costs.

                //The number of bites needed to store a 2-gram id type
                //Possible id types: 32^2 = 1,024
                //The number of bits needed to store the type is log_2(1,024) = 10
                //Using 16 bits for the speed efficiency, the total memory is not affected
                static const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS = 16;
                //The number of bites needed to store a 3-gram id type
                //Possible id types: 32^3 = 32,768
                //The number of bits needed to store the type is log_2(32,768) = 15
                //Using 16 bits for the speed efficiency, the total memory is not affected
                static const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS = 16;
                //The number of bites needed to store a 4-gram id type
                //Possible id types: 32^4 = 1,048,576
                //The number of bits needed to store the type is log_2(1,048,576) = 20
                //Using 24 bits for the speed efficiency, the total memory is not affected
                static const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS = 24;
                //The number of bites needed to store a 5-gram id type
                //Possible id types: 32^5 = 33,554,432
                //The number of bits needed to store the type is log_2(33,554,432) = 25
                //Using 32 bits for the speed efficiency, the total memory is not affected
                static const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS = 32;

                //The length of the M-gram id types in bits depending on the M-Gram level starting from 2.
                static constexpr uint8_t M_GRAM_ID_TYPE_LEN_BITS[] = {
                    0, 0,
                    M_GRAM_2_ID_TYPE_LEN_BITS,
                    M_GRAM_3_ID_TYPE_LEN_BITS,
                    M_GRAM_4_ID_TYPE_LEN_BITS,
                    M_GRAM_5_ID_TYPE_LEN_BITS
                };

                //Stores the maximum number of bits up to and including M-grams
                //of level 6.  We use sizeof (TShortId) as each wordId is of type
                //TShortId, and the maximum number of bits is thus defined by the
                //number of word_ids in the M-gram and their max size in bytes.
                static constexpr uint8_t M_GRAM_MAX_ID_LEN_BYTES[] = {
                    2 * sizeof (TShortId), // 2 TShortId values for 2 word ids 
                    3 * sizeof (TShortId), // 3 TShortId values for 3 word ids
                    4 * sizeof (TShortId), // 4 TShortId values for 4 word ids
                    5 * sizeof (TShortId), // 5 TShortId values for 5 word ids
                    6 * sizeof (TShortId), // 6 TShortId values for 6 word ids
                };

                //Define the basic type as an alias for the compressed M-Gram id
                typedef uint8_t * T_Gram_Id_Storage_Ptr;

                /**
                 * The compressed implementation of the M-gram id class
                 * Made in form of a namespace for the sake of minimizing the
                 * memory consumption
                 */
                namespace Comp_M_Gram_Id {

                    /**
                     * This method allows to re-initialize this class with a new M-gram id for the given M-gram.
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed
                     * theat this instance was created with the one argument constructor of this class allocated
                     * maximum needed memory for this level. Then the argument M-gram level must be smaller or
                     * equal to  the level this object was created with.
                     * @param word_ids the pointer to the array of word ids
                     * @param begin_idx the M-gram to create the id for
                     * @param num_word_ids the number of word ids
                     * @param m_p_gram_id the pointer to the data storage to be initialized
                     * @return true if the M-gram id could be created, otherwise false
                     */
                    void create_m_gram_id(const TShortId * word_ids,
                            const uint8_t begin_idx, const uint8_t num_word_ids,
                            T_Gram_Id_Storage_Ptr & m_p_gram_id);

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_m_gram_id(const TModelLevel level, T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        //Do the sanity check for against overflows
                        if (DO_SANITY_CHECKS && (level > M_GRAM_LEVEL_6)) {
                            stringstream msg;
                            msg << "T_Compressed_M_Gram_Id: Unsupported m-gram level: "
                                    << SSTR(level) << ", must be within ["
                                    << SSTR(M_GRAM_LEVEL_2) << ", "
                                    << SSTR(M_GRAM_LEVEL_6) << "], see M_GRAM_MAX_ID_LEN_BYTES array!";
                            throw Exception(msg.str());
                        }

                        //Allocate maximum memory that could be needed to store the given M-gram level id
                        m_p_gram_id = new uint8_t[M_GRAM_MAX_ID_LEN_BYTES[level - M_GRAM_LEVEL_2]];
                    }

                    /**
                     * Allows to destroy the M-Gram id if it is not NULL.
                     * @param m_p_gram_id the M-gram id pointer to destroy
                     */
                    static inline void destroy(T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        if (m_p_gram_id != NULL) {
                            LOG_DEBUG3 << "Deallocating a Compressed_M_Gram_Id: " <<  (void*) m_p_gram_id << END_LOG;
                            delete[] m_p_gram_id;
                        }
                    }

                    /**
                     * Allows to compare two M-Gram ids of a fixed M-gram level
                     * @param m_p_gram_id_one the first M-gram id
                     * @param m_p_gram_id_two the second M-gram id
                     * @return Negative value if one is smaller than two
                     *         Zero if one is equal to two
                     *         Positive value if one is larger than two
                     */
                    template<TModelLevel M_GRAM_LEVEL>
                    int compare(const T_Gram_Id_Storage_Ptr & m_p_gram_id_one, const T_Gram_Id_Storage_Ptr & m_p_gram_id_two);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "smaller" than the second, otherwise false
                     */
                    bool is_equal_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "smaller" than the second, otherwise false
                     */
                    bool is_less_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "larger" than the second, otherwise false
                     */
                    bool is_more_m_grams_id(const T_Gram_Id_Storage_Ptr & one, const T_Gram_Id_Storage_Ptr & two, const TModelLevel level);
                };
            }
        }
    }
}

#endif	/* MGRAM_HPP */

