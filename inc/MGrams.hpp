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

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

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
                 * This class is used to store the N-Gram data of the back-off Language Model.
                 */
                template<TModelLevel N, typename WordIndexType>
                class T_M_Gram {
                public:

                    //Stores the unknown word masks for the probability computations,
                    //up to and including 8-grams:
                    // 10000000, 01000000, 00100000, 00010000,
                    // 00001000, 00000100, 00000010, 00000001
                    static const uint8_t UNK_WORD_MASKS[];

                    //Stores the reference to the used word index
                    const WordIndexType & m_word_index;
                    //Stores the m-gram probability, the log_10 probability of the N-Gram Must be a negative value
                    TLogProbBackOff prob;
                    //Stores the m-gram log_10 back-off weight (probability) of the N-gram can be 0 is the probability is not available
                    TLogProbBackOff back_off;
                    //Stores, if needed, the m-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
                    TextPieceReader context;
                    //Stores the m-gram tokens
                    TextPieceReader tokens[N];
                    //Stores the m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                    TModelLevel level;

                    /**
                     * The basic constructor
                     * @param word_index the used word index
                     */
                    T_M_Gram(const WordIndexType & word_index) : m_word_index(word_index) {
                    }

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
                        return computeHash(beginFirstPtr, totalLen);
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

                    /**
                     * Converts the given tokens to ids and stores it in
                     * m_gram_word_ids. The ids are aligned to the beginning
                     * of the m_gram_word_ids[N-1] array.
                     * @param is_unk_flags if true then the unk word flags will be stored, otherwise not
                     * @param m_gram the m-gram tokens to convert to hashes
                     * @paam unk_word_flags the variable into which the word flags will be stored.
                     */
                    template<bool is_unk_flags>
                    inline void store_m_gram_word_ids(TShortId word_ids[N], uint8_t & unk_word_flags, const WordIndexType & word_index) const {
                        //The start index depends on the value M of the given M-Gram
                        TModelLevel idx = N - level;
                        LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(level) << "-gram:" << END_LOG;
                        for (TModelLevel i = 0; i != level; ++i) {
                            //Do not check whether the word was found or not, if it was not then the id is UNKNOWN_WORD_ID
                            word_ids[idx] = word_index.get_word_id(tokens[i]);
                            LOG_DEBUG1 << "wordId('" << tokens[i].str() << "') = " << SSTR(word_ids[idx]) << END_LOG;
                            if (is_unk_flags && (word_ids[idx] == WordIndexType::UNKNOWN_WORD_ID)) {
                                unk_word_flags |= UNK_WORD_MASKS[idx];
                            }
                            ++idx;
                        }
                        if (is_unk_flags) {
                            LOG_DEBUG << "The query unknown word flags are: " << bitset<NUM_BITS_IN_UINT_8>(unk_word_flags) << END_LOG;
                        }
                    }
                };

                template<TModelLevel N, typename WordIndexType>
                const uint8_t T_M_Gram<N, WordIndexType>::UNK_WORD_MASKS[] = {
                    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
                };

                //Make sure that there will be templates instantiated, at least for the given parameter values
                template class T_M_Gram<M_GRAM_LEVEL_MAX, BasicWordIndex>;
                template class T_M_Gram<M_GRAM_LEVEL_MAX, CountingWordIndex>;
                template class T_M_Gram<M_GRAM_LEVEL_MAX, TOptBasicWordIndex>;
                template class T_M_Gram<M_GRAM_LEVEL_MAX, TOptCountWordIndex>;

                /**
                 * This function allows to convert the M-gram tokens into a string representation. 
                 * @param gram  the M-Gram to work with
                 */
                template<TModelLevel N = M_GRAM_LEVEL_MAX, typename WordIndexType>
                inline string tokens_to_string(const T_M_Gram<N, WordIndexType> & gram) {
                    return tokens_to_string<N>(gram.tokens, gram.level);
                };

                /**
                 * The compressed implementation of the M-gram id class
                 * Made in form of a namespace for the sake of minimizing the
                 * memory consumption
                 */
                namespace M_Gram_Id {
                    //define the basic type block for the M-gram id
                    typedef uint8_t T_Gram_Id_Storage;

                    //Define the basic type as an alias for the compressed M-Gram id
                    typedef T_Gram_Id_Storage * T_Gram_Id_Storage_Ptr;

                    /**
                     * The basic constructor that allocates maximum memory
                     * needed to store the M-gram id of the given level.
                     * @param level the level of the M-grams this object will store id for.
                     * @param m_p_gram_id the pointer to initialize
                     */
                    static inline void allocate_m_gram_id(T_Gram_Id_Storage_Ptr & m_p_gram_id, uint8_t size) {
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

