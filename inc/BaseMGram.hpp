/* 
 * File:   BaseMGram.hpp
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
 * Created on October 28, 2015, 10:27 AM
 */

#ifndef BASEMGRAM_HPP
#define	BASEMGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"

#include "ByteMGramId.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

namespace uva {
    namespace smt {
        namespace tries {
            namespace m_grams {

                /**
                 * This class is the base class for all the M-gram classes used
                 */
                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY = M_GRAM_LEVEL_MAX>
                class T_Base_M_Gram {
                public:
                    //The type of the word id
                    typedef typename WordIndexType::TWordIdType TWordIdType;

                    //Define the corresponding M-gram id type
                    typedef m_gram_id::Byte_M_Gram_Id<TWordIdType> T_M_Gram_Id;

                    /**
                     * The basic constructor, is to be used when the M-gram level
                     * is known beforehand.
                     * Allows to set the actual m-gram level to a concrete value.
                     * @param word_index the used word index
                     * @param actual_level the actual level of the m-gram that will be used should be <= MAX_LEVEL_CAPACITY
                     */
                    T_Base_M_Gram(WordIndexType & word_index, TModelLevel actual_level)
                    : m_word_index(word_index), m_actual_level(actual_level),
                    m_actual_end_word_idx(actual_level - 1) {
                        if (DO_SANITY_CHECKS && (m_actual_level > MAX_LEVEL_CAPACITY)) {
                            stringstream msg;
                            msg << "The provided actual level: " << SSTR(m_actual_level)
                                    << " exceeds the maximum level capacity: "
                                    << SSTR(MAX_LEVEL_CAPACITY) << " of the T_Base_M_Gram class!";
                            throw Exception(msg.str());
                        }
                    }

                    /**
                     * The basic constructor, is to be used when the M-gram will
                     * actual level is not known beforehand - used e.g. in the query
                     * m-gram sub-class. The actual m-gram level is set to be
                     * undefined. Filling in the M-gram tokens is done elsewhere.
                     * @param word_index the used word index
                     */
                    T_Base_M_Gram(WordIndexType & word_index)
                    : m_word_index(word_index), m_actual_level(M_GRAM_LEVEL_UNDEF),
                    m_actual_end_word_idx(M_GRAM_LEVEL_UNDEF) {
                    }

                    /**
                     * Allows to get the word index used in this m-gram
                     * @return the word index
                     */
                    inline WordIndexType & get_word_index() const {
                        return m_word_index;
                    }

                    /**
                     * The basic to string conversion operator for the m-gram
                     */
                    inline operator string() const {
                        return tokens_to_string(m_tokens, m_actual_begin_word_idx, m_actual_end_word_idx);
                    };

                protected:
                    //Stores the m-gram tokens
                    TextPieceReader m_tokens[MAX_LEVEL_CAPACITY];

                    //Stores the reference to the used word index
                    WordIndexType & m_word_index;

                    //Stores the actual m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                    TModelLevel m_actual_level;

                    //These variables store the actual begin and end word index
                    //for all the words of this M-gram stored in the internal arrays
                    constexpr static TModelLevel m_actual_begin_word_idx = 0;
                    TModelLevel m_actual_end_word_idx;

                    /**
                     * Allows to create a new m-gram id for the sub-m-gram defined by the given of the method template parameters.
                     * For the argument reference to the id data pointer the following holds:
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                     * @param BEGIN_WORD_IDX the index of the first word in the sub-m-gram, indexes start with 0
                     * @param END_WORD_IDX the index of the last word in the sub-m-gram, indexes start with 0
                     * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                     *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                     * @param p_m_gram_id the reference to the M-gram id data pointer to be initialized with the M-gram id data, must be pre-allocated
                     */
                    template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX >
                    inline void create_m_gram_id(const TWordIdType word_ids[MAX_LEVEL_CAPACITY], T_Gram_Id_Data_Ptr & p_m_gram_id) const {
                        //Compute the number of words based on the input template parameters
                        constexpr TModelLevel NUMBER_OF_WORDS = END_WORD_IDX - BEGIN_WORD_IDX + 1;

                        LOG_DEBUG << "Computing sub " << SSTR(NUMBER_OF_WORDS) << "-gram id for the gram "
                                << "defined by the first, and the last word indexes: ["
                                << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;

                        //Create the M-gram id from the word ids.
                        T_M_Gram_Id::template create_m_gram_id<BEGIN_WORD_IDX, NUMBER_OF_WORDS>(word_ids, p_m_gram_id);

                        //Log the result
                        LOG_DEBUG << "Allocated " << NUMBER_OF_WORDS << "-gram id is: " << (void*) p_m_gram_id
                                << " for " << tokens_to_string(m_tokens, BEGIN_WORD_IDX, END_WORD_IDX) << END_LOG;
                    }

                    /**
                     * Allows to compute the hash for the given sub-m-gram that is defined by the
                     * given of the method template parameters. The hash is computed incrementally
                     * from the last word id and then up until the first word idx of the sub-m-gram
                     * defined by the template parameters. The hash computation is stopped as soon
                     * as an <unk> word is encountered.
                     * @param BEGIN_WORD_IDX the index of the first word in the sub-m-gram, indexes start with 0
                     * @param END_WORD_IDX the index of the last word in the sub-m-gram, indexes start with 0
                     * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                     *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                     * @param hash_values the array of hash values to be filled in with hashes
                     * @return true if all the requested word hashes could be computed otherwise false
                     */
                    template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX >
                    inline bool compute_hashes(const TWordIdType word_ids[MAX_LEVEL_CAPACITY], uint64_t hash_values[MAX_LEVEL_CAPACITY]) const {
                        //Declare and default initialize the result variable
                        bool result = true;
                        //Compute the number of words based on the input template parameters
                        constexpr TModelLevel NUMBER_OF_WORDS = END_WORD_IDX - BEGIN_WORD_IDX + 1;

                        LOG_DEBUG << "Computing sub " << SSTR(NUMBER_OF_WORDS) << "-gram hash for the gram "
                                << "defined by the first, and the last word indexes: ["
                                << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;

                        //First check if the end word is not unknown, if yes then there is no need to compute hashes
                        TModelLevel curr_idx = END_WORD_IDX;
                        if (word_ids[curr_idx] != WordIndexType::UNKNOWN_WORD_ID) {
                            //If the word is not unknown then the first hash, the word's hash is its id
                            hash_values[curr_idx] = word_ids[curr_idx];

                            //Iterate through the remaining word ids, if any, and build-up hashes
                            if (curr_idx > BEGIN_WORD_IDX) {
                                do {
                                    //Decrement the word id
                                    curr_idx--;

                                    //Check if the next word id is unknown, if yes then just stop iterations
                                    if (word_ids[curr_idx] != WordIndexType::UNKNOWN_WORD_ID) {
                                        //Incrementally build up hash, using the previous hash value and the next word id
                                        hash_values[curr_idx] = combine_hash(word_ids[curr_idx], hash_values[curr_idx + 1]);
                                    } else {
                                        //We did not compute all the required hashed because of <unk>
                                        result = false;
                                        //Stop iterations
                                        break;
                                    }
                                } while (curr_idx != BEGIN_WORD_IDX);
                            }

                            //Log the result
                            LOG_DEBUG << "Computed " << NUMBER_OF_WORDS << "-gram hash is: " << word_ids[curr_idx]
                                    << " for the maximum sub-m-gram: " << tokens_to_string(m_tokens, curr_idx, END_WORD_IDX) << END_LOG;
                        } else {
                            //We did not compute all the required hashed because of <unk>
                            result = false;
                            
                            //Log the result
                            LOG_DEBUG << "Computed " << NUMBER_OF_WORDS << "-gram hash is: NONE, the word: "
                                    << tokens_to_string(m_tokens, END_WORD_IDX, END_WORD_IDX) << " with index "
                                    << SSTR(END_WORD_IDX) << " is an <unk>word!" << END_LOG;
                        }

                        return result;
                    }
                };
            }
        }
    }
}

#endif	/* BASEMGRAM_HPP */

