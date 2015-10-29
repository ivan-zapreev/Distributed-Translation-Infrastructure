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
                    T_Base_M_Gram(WordIndexType & word_index, TModelLevel actual_level) : m_word_index(word_index), m_actual_level(actual_level) {
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
                    T_Base_M_Gram(WordIndexType & word_index) : m_word_index(word_index), m_actual_level(M_GRAM_LEVEL_UNDEF) {
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
                        return tokens_to_string(m_tokens, get_first_word_index(MAX_LEVEL_CAPACITY, m_actual_level, false), get_last_word_index(MAX_LEVEL_CAPACITY, m_actual_level, false));
                    };

                protected:
                    //Stores the m-gram tokens
                    TextPieceReader m_tokens[MAX_LEVEL_CAPACITY];

                    //Stores the reference to the used word index
                    WordIndexType & m_word_index;

                    //Stores the actual m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                    TModelLevel m_actual_level;

                    /**
                     * Allows to get the first word index within the tokens of word id array for the given parameters
                     * @param PREFIX_LENGTH the considered m-gram prefix length
                     * @param CURR_LEVEL the current sub-m-gram  level
                     * @return the array index for the first word of this m-gram
                     */
                    constexpr static inline TModelLevel get_first_word_index(const TModelLevel PREFIX_LENGTH, const TModelLevel CURR_LEVEL){
                        return (PREFIX_LENGTH - CURR_LEVEL);
                    }

                    /**
                     * Allows to get the last word index within the tokens of word id array for the given parameters
                     * @param PREFIX_LENGTH the considered m-gram prefix length
                     * @param CURR_LEVEL the current sub-m-gram  level
                     * @return the array index for the last word of this m-gram
                     */
                    constexpr static inline TModelLevel get_last_word_index(const TModelLevel PREFIX_LENGTH, const TModelLevel CURR_LEVEL) {
                        return (PREFIX_LENGTH - 1);
                    }

                    /**
                     * Allows to get the length of the sub-m-gram for the given parameters
                     * @param PREFIX_LENGTH the considered m-gram prefix length
                     * @param CURR_LEVEL the current sub-m-gram  level
                     * @return the length of the sub-m-gram
                     */
                    constexpr static inline TModelLevel get_sub_gram_length(const TModelLevel PREFIX_LENGTH, const TModelLevel CURR_LEVEL) {
                        return CURR_LEVEL;
                    }

                    /**
                     * Allows to create a new m-gram id for the sub-m-gram defined by the given of the method template parameters.
                     * For the argument reference to the id data pointer the following holds:
                     * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                     * as needed to store the given id.
                     * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                     * @param PREFIX_LENGTH the length of the currently considered M-gram prefix
                     * @param CURR_LEVEL the level of the considered sub-m-gram in the considered m-gram prefix
                     * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                     *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                     * @param p_m_gram_id the reference to the M-gram id data pointer to be initialized with the M-gram id data, must be pre-allocated
                     */
                    template<TModelLevel PREFIX_LENGTH, TModelLevel CURR_LEVEL >
                    inline void create_m_gram_id(const TWordIdType word_ids[MAX_LEVEL_CAPACITY], T_Gram_Id_Data_Ptr & p_m_gram_id) const {
                        //Compute the first index based on the input template parameters
                        constexpr TModelLevel FIRST_WORD_IDX = get_first_word_index(PREFIX_LENGTH, CURR_LEVEL);
                        //Compute the length based on the input template parameters
                        constexpr TModelLevel SUB_GRAM_LEN = get_sub_gram_length(PREFIX_LENGTH, CURR_LEVEL);

                        LOG_DEBUG << "Computing sub " << SSTR(CURR_LEVEL) << "-gram id for the m-gram defined by the first"
                                << " word index: " << SSTR(FIRST_WORD_IDX) << ", and length: " << SSTR(SUB_GRAM_LEN) << END_LOG;

                        //Create the M-gram id from the word ids.
                        T_M_Gram_Id::template create_m_gram_id<FIRST_WORD_IDX, SUB_GRAM_LEN>(word_ids, p_m_gram_id);

                        //Compute the last index based on the input template parameters, is only needed for logging
                        constexpr TModelLevel LAST_WORD_IDX = get_last_word_index(PREFIX_LENGTH, CURR_LEVEL);
                        LOG_DEBUG << "Allocated " << CURR_LEVEL << "-gram id is: " << (void*) p_m_gram_id
                                << " for " << tokens_to_string(m_tokens, FIRST_WORD_IDX, LAST_WORD_IDX) << END_LOG;
                    }

                    /**
                     * Allows to compute the hash for the given sub-m-gram that is defined by the
                     * given of the method template parameters.
                     * The hash computation is stopped as soon as an <unk> word is encountered.
                     * The hash is computed incrementally from the last word id and then up until
                     * the first word id of the sub-m-gram defined by the template parameters.
                     * @param PREFIX_LENGTH the length of the currently considered M-gram prefix
                     * @param CURR_LEVEL the level of the considered sub-m-gram in the considered m-gram prefix
                     * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                     *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                     * @param hash_values the array of hash values to be filled in with hashes
                     */
                    template<TModelLevel PREFIX_LENGTH, TModelLevel C URR_LEVEL>
                    inline void compute_hashes(const TWordIdType word_ids[MAX_LEVEL_CAPACITY], uint64_t hash_values[MAX_LEVEL_CAPACITY]) const {
                        //Compute the first index based on the input template parameters
                        constexpr TModelLevel FIRST_WORD_IDX = get_first_word_index(PREFIX_LENGTH, CURR_LEVEL);
                        //Compute the last index based on the input template parameters
                        constexpr TModelLevel LAST_WORD_IDX = get_last_word_index(PREFIX_LENGTH, CURR_LEVEL);

                        LOG_DEBUG << "Computing sub " << SSTR(CURR_LEVEL) << "-gram hash for the m-gram "
                                << "defined by the first, and the last word indexes: ["
                                << SSTR(FIRST_WORD_IDX) << ", " << SSTR(LAST_WORD_IDX) << "]" << END_LOG;
                        
                        //ToDo: Implement
                    }

                };

            }
        }
    }
}

#endif	/* BASEMGRAM_HPP */

