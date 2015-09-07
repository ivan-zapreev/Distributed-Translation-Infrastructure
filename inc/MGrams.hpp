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

                /**
                 * The compressed implementation of the M-gram id class
                 */
                template<TModelLevel M_GRAM_LEVEL>
                class T_Compressed_M_Gram_Id {
                public:

                    //The memory in bits needed to store different M-gram id types in the M-gram id byte arrays
                    static const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS;

                    T_Compressed_M_Gram_Id(const T_M_Gram & gram, const AWordIndex * p_word_idx);

                    virtual ~T_Compressed_M_Gram_Id() {
                        if (m_gram_id != NULL) {
                            delete[] m_gram_id;
                        }
                    }

                protected:
                    //This should store the unique identifier of the M-gram allocated with a new operator
                    uint8_t * m_gram_id;

                    /**
                     * Allows to extract the M-gram id length in bytes
                     * Note that this method is applicable only for M-grams with M <= 6;
                     * @param m_gram_id the M-gram id to extract the length for
                     * @param level the M-gram level
                     * @return the M-gram id length in bytes
                     */
                    static uint8_t get_m_gram_id_len(const uint8_t * & m_gram_id);

                    template<bool IS_LESS, TModelLevel N> friend bool compare(const T_Compressed_M_Gram_Id<N> & one, const T_Compressed_M_Gram_Id<N> & two);

                };

                /**
                 * Allows to compare two M-Gram ids depending on the template flag it is a different operator
                 * @param IS_LESS if true the it is a is_less compare, if false then is_more
                 * @param one the first M-gram id
                 * @param two the second M-gram id
                 * @return true if "one < two" otherwise false
                 */
                template<bool IS_LESS, TModelLevel M_GRAM_LEVEL>
                static inline bool compare(
                        const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & one,
                        const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & two) {
                    //Get the M-gram type ids
                    TShortId type_one;
                    //ToDo: Implement
                    TShortId type_two;
                    //ToDo: Implement

                    throw Exception("ToDo: static inline bool compare(const T_M_Gram_Id & one, const T_M_Gram_Id & two, const TModelLevel level)");

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
                            const uint8_t len = T_Compressed_M_Gram_Id<M_GRAM_LEVEL>::get_m_gram_id_len(one.m_gram_id);

                            //Start comparing the ids byte by byte but not from the fist
                            //bytes as this is where the id type information is stored 

                            //ToDo: Implement
                        }
                    }
                };

                template<TModelLevel M_GRAM_LEVEL>
                bool operator<(const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & one, const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & two) {
                    return compare<true>(one, two);
                }

                template<TModelLevel M_GRAM_LEVEL>
                bool operator>(const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & one, const T_Compressed_M_Gram_Id<M_GRAM_LEVEL> & two) {
                    return compare<false>(one, two);
                }

            }
        }
    }
}

#endif	/* MGRAM_HPP */

