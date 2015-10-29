/* 
 * File:   QueryMGram.hpp
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
 * Created on October 28, 2015, 10:28 AM
 */

#ifndef QUERYMGRAM_HPP
#define	QUERYMGRAM_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "BaseMGram.hpp"

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
                 * This class is used to represent the N-Gram that will be queried against the language model.
                 */
                template<typename WordIndexType, TModelLevel MAX_LEVEL_CAPACITY = M_GRAM_LEVEL_MAX>
                class T_Query_M_Gram : public T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> {
                public:
                    //The type of the word id
                    typedef typename WordIndexType::TWordIdType TWordIdType;
                    //Define the corresponding M-gram id type
                    typedef m_gram_id::Byte_M_Gram_Id<TWordIdType> T_M_Gram_Id;
                    //Define the base class type
                    typedef T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY> BASE;

                private:

                    /**
                     * This constructor is made private as it is not to be used
                     */
                    T_Query_M_Gram(WordIndexType & word_index, TModelLevel actual_level)
                    : T_Base_M_Gram<WordIndexType, MAX_LEVEL_CAPACITY>(word_index, actual_level) {
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
                                    //Stop iterating if the reached the beginning of the m-gram
                                } while (curr_idx != BEGIN_WORD_IDX);
                            }

                            //Log the result
                            LOG_DEBUG << "Computed " << NUMBER_OF_WORDS << "-gram hash is: " << word_ids[curr_idx]
                                    << " for the maximum sub-m-gram: " << tokens_to_string(BASE::m_tokens, curr_idx, END_WORD_IDX) << END_LOG;
                        } else {
                            //We did not compute all the required hashed because of <unk>
                            result = false;

                            //Log the result
                            LOG_DEBUG << "Computed " << NUMBER_OF_WORDS << "-gram hash is: NONE, the word: "
                                    << tokens_to_string(BASE::m_tokens, END_WORD_IDX, END_WORD_IDX) << " with index "
                                    << SSTR(END_WORD_IDX) << " is an <unk>word!" << END_LOG;
                        }

                        return result;
                    }
                };

            }
        }
    }
}

#endif	/* QUERYMGRAM_HPP */

