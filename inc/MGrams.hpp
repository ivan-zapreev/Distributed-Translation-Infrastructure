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
                 * This function allows to convert the M-gram tokens into a string representation. 
                 * @param gram  the M-Gram to work with
                 */
                template<TModelLevel N = M_GRAM_LEVEL_MAX>
                inline string tokensToString(const T_M_Gram & gram) {
                    return tokensToString<N>(gram.tokens, gram.level);
                };

                /**
                 * The compressed implementation of the M-gram id class
                 */
                class T_Compressed_M_Gram_Id {
                public:

                    //The memory in bits needed to store different M-gram id types in the M-gram id byte arrays
                    static const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS;
                    static const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS;

                    /**
                     * The basic constructor to create an M-Gram id
                     * @param gram the M-gram to create the id for
                     * @param p_word_idx the word index
                     * @throw Exception if an id could not be created.
                     */
                    T_Compressed_M_Gram_Id(const T_M_Gram & gram, const AWordIndex * p_word_idx);

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     */
                    T_Compressed_M_Gram_Id(const TModelLevel level);

                    /**
                     * This method allows to re-initialize this class with a new M-gram id for the given M-gram.
                     * The memory will not be re-allocated to it is assumed theat this instance was created with
                     * the one argument constructor of this class allocated maximum needed memory for this level.
                     * The argument M-gram level must be smaller or equal to  the level this object was created with.
                     * @param gram the M-gram to create the id for
                     * @param p_word_idx the word index
                     * @return true if the M-gram id could be created, otherwise false
                     */
                    bool create_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx);

                    virtual ~T_Compressed_M_Gram_Id() {
                        if (m_gram_id != NULL) {
                            delete[] m_gram_id;
                        }
                    }

                    /**
                     * Allows to extract the M-gram id length in bytes
                     * @return the M-gram id length in bytes
                     */
                    template<TModelLevel M_GRAM_LEVEL>
                    inline uint8_t get_m_gram_id_len() {
                        return get_m_gram_id_len<M_GRAM_LEVEL>(m_gram_id);
                    }

                    /**
                     * Allows to get the M-gram id type
                     * @param id_type [out] the M-gram id type
                     */
                    template<TModelLevel M_GRAM_LEVEL>
                    void get_m_gram_id_type(uint8_t & id_type);

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
                    template<TModelLevel M_GRAM_LEVEL>
                    static uint8_t get_m_gram_id_len(const uint8_t * m_gram_id);
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
                        const T_Compressed_M_Gram_Id & one,
                        const T_Compressed_M_Gram_Id & two) {
                    //Get the M-gram type ids
                    TShortId type_one;
                    one.get_m_gram_id_type<M_GRAM_LEVEL>(type_one);
                    TShortId type_two;
                    two.get_m_gram_id_type<M_GRAM_LEVEL>(type_two);

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
                            const uint8_t len = one.get_m_gram_id_len<M_GRAM_LEVEL>();

                            //Start comparing the ids byte by byte but not from the fist
                            //bytes as this is where the id type information is stored 

                            //ToDo: Implement

                            throw Exception("ToDo: static inline bool compare(const T_M_Gram_Id & one, const T_M_Gram_Id & two, const TModelLevel level)");
                        }
                    }
                };
            }
        }
    }
}

#endif	/* MGRAM_HPP */

