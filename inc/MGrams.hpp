/* 
 * File:   MGrams.hpp
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

#ifndef MGRAMS_HPP
#define	MGRAMS_HPP

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
                    inline uint32_t sub_hash(const TModelLevel begin_idx, const TModelLevel end_idx) const {
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
                        return computeRSHash(beginFirstPtr, totalLen);
                    }

                    /**
                     * This function allows to compute the hash of the M-Gram suffix
                     * starting from and including the word on the given index. It
                     * assumes, which should hold, that the memory pointed by the
                     * tokens is continuous.
                     * @param begin_idx  the index of the first word in tokens array
                     * @return the hash value of the given token
                     */
                    inline uint32_t suffix_hash(const TModelLevel begin_idx) const {
                        const TModelLevel end_idx = level - 1;
                        return sub_hash(begin_idx, end_idx);
                    }

                    /**
                     * This function allows to compute the hash of the given M-Gram
                     * It assumes, which should hold, that the memory pointed by the tokens is continuous
                     * @return the hash value of the given token
                     */
                    inline uint32_t hash() const {
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

                /**
                 * The compressed implementation of the M-gram id class
                 * Made in form of a namespace for the sake of minimizing the
                 * memory consumption
                 */
                namespace M_Gram_Id {

                    //Define the basic type as an alias for the compressed M-Gram id
                    typedef uint8_t * T_Gram_Id_Storage_Ptr;

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_m_gram_id(uint8_t size, T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        //Allocate maximum memory that could be needed to store the given M-gram level id
                        m_p_gram_id = new uint8_t[size];
                        LOG_DEBUG3 << "Allocating a M_Gram_Id: " << (void*) m_p_gram_id << " of size " << (uint32_t) size << END_LOG;
                    }

                    /**
                     * Allows to destroy the M-Gram id if it is not NULL.
                     * @param m_p_gram_id the M-gram id pointer to destroy
                     */
                    static inline void destroy(T_Gram_Id_Storage_Ptr & m_p_gram_id) {
                        if (m_p_gram_id != NULL) {
                            LOG_DEBUG3 << "Deallocating a M_Gram_Id: " << (void*) m_p_gram_id << END_LOG;
                            delete[] m_p_gram_id;
                        }
                    }

                }
            }
        }
    }
}

#endif	/* MGRAM_HPP */

