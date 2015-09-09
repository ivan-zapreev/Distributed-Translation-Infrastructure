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

                //Define the basic type as an alias for the compressed M-Gram id
                typedef uint8_t * T_Comp_M_Gram_Id_Ptr;

                /**
                 * The compressed implementation of the M-gram id class
                 * Made in form of a namespace for the sake of minimizing the
                 * memory consumption
                 */
                namespace Comp_M_Gram_Id {

                    //The memory in bits needed to store different M-gram id types in
                    //the M-gram id byte arrays
                    //ToDo: These are the minimum values, if used then we use fewest memory
                    //but the data is not byte aligned. Therefore some bit copying operations
                    //are not done efficiently. Perhaps it is worth trying to round these
                    //values up to full bytes, this can improve performance @ some memory costs.

                    //The number of bites needed to store a 2-gram id type
                    //Possible id types: 32^2 = 1,024
                    //The number of bits needed to store the type is log_2(1,024) = 10
                    const uint8_t M_GRAM_2_ID_TYPE_LEN_BITS = 10;
                    //The number of bites needed to store a 3-gram id type
                    //Possible id types: 32^3 = 32,768
                    //The number of bits needed to store the type is log_2(32,768) = 15
                    const uint8_t M_GRAM_3_ID_TYPE_LEN_BITS = 15;
                    //The number of bites needed to store a 4-gram id type
                    //Possible id types: 32^4 = 1,048,576
                    //The number of bits needed to store the type is log_2(1,048,576) = 20
                    const uint8_t M_GRAM_4_ID_TYPE_LEN_BITS = 20;
                    //The number of bites needed to store a 5-gram id type
                    //Possible id types: 32^5 = 33,554,432
                    //The number of bits needed to store the type is log_2(33,554,432) = 25
                    const uint8_t M_GRAM_5_ID_TYPE_LEN_BITS = 25;

                    /**
                     * The basic constructor to create an M-Gram id
                     * @param gram the M-gram to create the id for
                     * @param p_word_idx the word index
                     * @param m_p_gram_id the pointer to initialize
                     * @throw Exception if an id could not be created.
                     */
                    void allocate_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, T_Comp_M_Gram_Id_Ptr & m_p_gram_id);

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    void allocate_m_gram_id(const TModelLevel level, T_Comp_M_Gram_Id_Ptr & m_p_gram_id);

                    /**
                     * Allows to destroy the M-Gram id if it is not NULL.
                     * @param m_p_gram_id the M-gram id pointer to destroy
                     */
                    static inline void destroy(T_Comp_M_Gram_Id_Ptr & m_p_gram_id) {
                        if (m_p_gram_id != NULL) {
                            LOG_INFO3 << "Deallocating a Compressed_M_Gram_Id: " << SSTR((void *) m_p_gram_id) << END_LOG;
                            delete[] m_p_gram_id;
                        }
                    }

                    /**
                     * This method allows to re-initialize this class with a new M-gram id for the given M-gram.
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed
                     * theat this instance was created with the one argument constructor of this class allocated
                     * maximum needed memory for this level. Then the argument M-gram level must be smaller or
                     * equal to  the level this object was created with.
                     * @param gram the M-gram to create the id for
                     * @param p_word_idx the word index
                     * @param m_p_gram_id the pointer to the data storage to be initialized
                     * @return true if the M-gram id could be created, otherwise false
                     */
                    bool set_m_gram_id(const T_M_Gram & gram, const AWordIndex * p_word_idx, T_Comp_M_Gram_Id_Ptr m_p_gram_id);

                    /**
                     * Allows to compare two M-Gram ids depending on the template flag it is a different operator
                     * @param IS_LESS if true the it is a is_less compare, if false then is_more
                     * @param one the first M-gram id
                     * @param two the second M-gram id
                     * @return true if "one < two" otherwise false
                     */
                    template<bool IS_LESS, TModelLevel M_GRAM_LEVEL>
                    static bool compare(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two);
                    
                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "smaller" than the second, otherwise false
                     */
                    bool is_less_m_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two, const TModelLevel level);

                    /**
                     * This is a fore-declaration of the function that can compare two M-gram ids of the same given level
                     * @param one the first M-gram to compare
                     * @param two the second M-gram to compare
                     * @param level the M-grams' level M
                     * @return true if the first M-gram is "larger" than the second, otherwise false
                     */
                    bool is_more_m_grams_id(const T_Comp_M_Gram_Id_Ptr & one, const T_Comp_M_Gram_Id_Ptr & two, const TModelLevel level);
                }
            }
        }
    }
}

#endif	/* MGRAM_HPP */

