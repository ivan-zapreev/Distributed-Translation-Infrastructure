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
#define QUERYMGRAM_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"

#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/math_utils.hpp"
#include "common/utils/exceptions.hpp"

#include "server/common/models/phrase_uid.hpp"

#include "server/lm/mgrams/BaseMGram.hpp"
#include "server/lm/mgrams/ByteMGramId.hpp"
#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        /**
                         * This class is used to represent the N-Gram that will be queried against the language model.
                         */
                        template<typename WordIndexType, TModelLevel MAX_LEVEL = M_GRAM_LEVEL_MAX>
                        class T_Query_M_Gram : public T_Base_M_Gram<WordIndexType, MAX_LEVEL> {
                        public:
                            //The type of the word id
                            typedef typename WordIndexType::TWordIdType TWordIdType;
                            //Define the corresponding M-gram id type
                            typedef m_gram_id::Byte_M_Gram_Id<TWordIdType, MAX_LEVEL> T_M_Gram_Id;
                            //Define the base class type
                            typedef T_Base_M_Gram<WordIndexType, MAX_LEVEL> BASE;

                            /**
                             * The basic constructor, is to be used when the M-gram will
                             * actual level is not known beforehand - used e.g. in the query
                             * m-gram sub-class. The actual m-gram level is set to be
                             * undefined. Filling in the M-gram tokens is done elsewhere.
                             * @param word_index the used word index
                             */
                            T_Query_M_Gram(WordIndexType & word_index)
                            : T_Base_M_Gram<WordIndexType, MAX_LEVEL>(word_index) {
                            }

                            /**
                             * Allows to retrieve the hash value for the sub-m-gram 
                             * defined by the parameters
                             * @param begin_word_idx the begin word index of the sub-m-gram
                             * @param end_word_idx the end word index of the sub-m-gram
                             * @return the hash value for the given sub-m-gram
                             */
                            inline uint64_t get_hash(TModelLevel begin_word_idx, const TModelLevel end_word_idx) const {
                                //Define the reference to the previous level
                                TModelLevel & prev_level_ref = const_cast<TModelLevel &> (m_hash_level_row[begin_word_idx]);

                                LOG_DEBUG1 << "Getting hash values for begin/end index: " << SSTR(begin_word_idx)
                                        << "/" << SSTR(end_word_idx) << ", the previous computed begin level "
                                        << "is: " << SSTR(prev_level_ref) << END_LOG;

                                //Define the reference to the hash row
                                uint64_t(& hash_row_ref)[MAX_LEVEL] = const_cast<uint64_t(&)[MAX_LEVEL]> (m_hash_matrix[begin_word_idx]);

                                //Compute the current level
                                const TModelLevel curr_level = CURR_LEVEL_MAP[begin_word_idx][end_word_idx];
                                //Check if the given hash is already available.
                                if (curr_level > prev_level_ref) {
                                    //Check if there has its been computed before for this row
                                    if (prev_level_ref == M_GRAM_LEVEL_UNDEF) {
                                        //If there has not been anything computed yet,
                                        //then first initialize the starting word
                                        hash_row_ref[begin_word_idx] = BASE::m_word_ids[begin_word_idx];

                                        LOG_DEBUG1 << "word[" << SSTR(begin_word_idx) << "] = "
                                                << BASE::m_word_ids[begin_word_idx]
                                                << ", hash[" << SSTR(begin_word_idx) << "] = "
                                                << hash_row_ref[begin_word_idx] << END_LOG;

                                        ++begin_word_idx;
                                    } else {
                                        //This is the case of at least a bi-gram, but the actual
                                        //begin word index is the one stored from before
                                        begin_word_idx += prev_level_ref;
                                    }

                                    //Iterate on and compute the subsequent hashes, if any
                                    for (; begin_word_idx <= end_word_idx; ++begin_word_idx) {
                                        //Incrementally build up hash, using the previous hash value and the next word id
                                        hash_row_ref[begin_word_idx] = combine_phrase_uids(hash_row_ref[begin_word_idx - 1], BASE::m_word_ids[begin_word_idx]);

                                        LOG_DEBUG1 << "hash[" << SSTR(begin_word_idx) << "] = combine( word["
                                                << SSTR(begin_word_idx) << "] = " << BASE::m_word_ids[begin_word_idx]
                                                << ", hash[" << SSTR(begin_word_idx - 1) << "] = "
                                                << hash_row_ref[begin_word_idx - 1] << " ) = "
                                                << hash_row_ref[begin_word_idx] << END_LOG;
                                    }
                                    //Set the processed level 
                                    prev_level_ref = curr_level;
                                }

                                LOG_DEBUG1 << "Resulting hash value: " << hash_row_ref[end_word_idx] << END_LOG;

                                //Return the hash value that must have been pre-computed
                                return hash_row_ref[end_word_idx];
                            }

                            /**
                             * For the given N-gram, for some level M <=N , this method
                             * allows to give the string of the object for which the
                             * probability is computed, e.g.:
                             * N-gram = "word1" -> result = "word1"
                             * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                             * for the first M tokens of the N-gram
                             * @param level the level M of the sub-m-gram prefix to work with
                             * @return the resulting string
                             */
                            inline string get_mgram_prob_str(const TModelLevel level) const {
                                if (level == M_GRAM_LEVEL_UNDEF) {
                                    return "<none>";
                                } else {
                                    if (level == M_GRAM_LEVEL_1) {
                                        const TextPieceReader & token = BASE::m_tokens[BASE::m_actual_begin_word_idx];
                                        return token.str().empty() ? "<empty>" : token.str();
                                    } else {
                                        const TModelLevel end_word_idx = (level - 1);
                                        string result = BASE::m_tokens[end_word_idx].str() + " |";
                                        for (TModelLevel idx = BASE::m_actual_begin_word_idx; idx != end_word_idx; idx++) {
                                            result += string(" ") + BASE::m_tokens[idx].str();
                                        }
                                        return result;
                                    }
                                }
                            }

                            /**
                             * For the given N-gram, this method allows to give the string 
                             * of the object for which the probability is computed, e.g.:
                             * N-gram = "word1" -> result = "word1"
                             * N-gram = "word1 word2 word3" -> result = "word1 word2 word3"
                             * @return the resulting string
                             */
                            inline string get_mgram_prob_str() const {
                                if (BASE::m_actual_level == M_GRAM_LEVEL_UNDEF) {
                                    return "<none>";
                                } else {
                                    if (BASE::m_actual_level == M_GRAM_LEVEL_1) {
                                        const TextPieceReader & token = BASE::m_tokens[BASE::m_actual_begin_word_idx];
                                        return token.str().empty() ? "<empty>" : token.str();
                                    } else {
                                        string result;
                                        for (TModelLevel idx = BASE::m_actual_begin_word_idx; idx <= BASE::m_actual_end_word_idx; idx++) {
                                            result += BASE::m_tokens[idx].str() + string(" ");
                                        }
                                        return result.substr(0, result.length() - 1);
                                    }
                                }
                            }

                            /**
                             * Tokenise a given piece of text into a space separated list of text pieces.
                             * @param text the piece of text to tokenise
                             * @param gram the gram container to put data into
                             */
                            inline void set_m_gram_from_text(TextPieceReader &text) {
                                //Set all the "computed hash level" flags to "undefined"
                                memset(m_hash_level_row, M_GRAM_LEVEL_UNDEF, MAX_LEVEL * sizeof (TModelLevel));

                                //Initialize the actual level with undefined (zero)
                                BASE::m_actual_level = M_GRAM_LEVEL_UNDEF;

                                //Read the tokens one by one backwards and decrement the index
                                while (text.get_first_space(BASE::m_tokens[BASE::m_actual_level])) {
                                    LOG_DEBUG1 << "Obtained the " << BASE::m_actual_level << "'th m-gram token: ___"
                                            << BASE::m_tokens[BASE::m_actual_level] << "___" << END_LOG;

                                    //Retrieve the word id
                                    BASE::m_word_ids[BASE::m_actual_level] = BASE::m_word_index.get_word_id(BASE::m_tokens[BASE::m_actual_level]);

                                    LOG_DEBUG2 << "The word: '" << BASE::m_tokens[BASE::m_actual_level] << "' is: "
                                            << SSTR(BASE::m_word_ids[BASE::m_actual_level]) << "!" << END_LOG;

                                    //Increment the counter
                                    ++BASE::m_actual_level;

                                    LOG_DEBUG2 << "The current m-gram level is: " << BASE::m_actual_level
                                            << ", the maximum is: " << MAX_LEVEL << END_LOG;

                                    ASSERT_SANITY_THROW((BASE::m_actual_level > MAX_LEVEL),
                                            string("A broken N-gram query: ") + ((string) * this) +
                                            string(", level: ") + to_string(BASE::m_actual_level));
                                }

                                //Set the actual end word index
                                BASE::m_actual_end_word_idx = BASE::m_actual_level - 1;

                                ASSERT_SANITY_THROW(((BASE::m_actual_level < M_GRAM_LEVEL_1) ||
                                        (BASE::m_actual_level > MAX_LEVEL)),
                                        string("A broken N-gram query: ") + ((string) * this) +
                                        string(", level: ") + to_string(BASE::m_actual_level));
                            }

                        private:
                            //Stores the hash computed flags
                            TModelLevel m_hash_level_row[MAX_LEVEL];
                            //Stores the computed hash values
                            uint64_t m_hash_matrix[MAX_LEVEL][MAX_LEVEL];

                            /**
                             * This constructor is made private as it is not to be used
                             */
                            T_Query_M_Gram(WordIndexType & word_index, TModelLevel actual_level)
                            : T_Base_M_Gram<WordIndexType, MAX_LEVEL>(word_index, actual_level) {
                            }

                        };
                    }
                }
            }
        }
    }
}

#endif /* QUERYMGRAM_HPP */

