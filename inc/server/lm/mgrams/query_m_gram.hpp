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
#include "m_gram_payload.hpp"

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {
                        
                        //Make local constant declarations for the sake of brevity
                        static constexpr phrase_length QUERY_M_GRAM_MAX_LEN = LM_MAX_QUERY_LEN;

                        /**
                         * This class is used to represent the N-Gram that will be queried against the language model.
                         */
                        class query_m_gram : public phrase_base<QUERY_M_GRAM_MAX_LEN, LM_M_GRAM_LEVEL_MAX> {
                        public:
                            //Define the base class type
                            typedef phrase_base<QUERY_M_GRAM_MAX_LEN, LM_M_GRAM_LEVEL_MAX> BASE;

                            /**
                             * The basic constructor, is to be used when the M-gram will
                             * actual level is not known beforehand - used e.g. in the query
                             * m-gram sub-class. The actual m-gram level is set to be
                             * undefined. Filling in the M-gram tokens is done elsewhere.
                             * @param word_index the used word index
                             */
                            query_m_gram() : phrase_base<QUERY_M_GRAM_MAX_LEN, LM_M_GRAM_LEVEL_MAX>() {
                            }

                            /**
                             * Allows to retrieve the hash value for the sub-m-gram 
                             * defined by the parameters
                             * @param begin_word_idx the begin word index of the sub-m-gram
                             * @param end_word_idx the end word index of the sub-m-gram
                             * @return the hash value for the given sub-m-gram
                             */
                            inline uint64_t get_hash(phrase_length begin_word_idx, const phrase_length end_word_idx) const {
                                //Define the reference to the previous level
                                phrase_length & prev_level_ref = const_cast<phrase_length &> (m_hash_level_row[begin_word_idx]);

                                LOG_DEBUG1 << "Getting hash values for begin/end index: " << SSTR(begin_word_idx)
                                        << "/" << SSTR(end_word_idx) << ", the previous computed begin level "
                                        << "is: " << SSTR(prev_level_ref) << END_LOG;

                                //Define the reference to the hash row
                                uint64_t(& hash_row_ref)[QUERY_M_GRAM_MAX_LEN] = const_cast<uint64_t(&)[QUERY_M_GRAM_MAX_LEN]> (m_hash_matrix[begin_word_idx]);

                                //Compute the current level
                                const phrase_length curr_level = CURR_LEVEL_MAP[begin_word_idx][end_word_idx];
                                //Check if the given hash is already available.
                                if (curr_level > prev_level_ref) {
                                    //Check if there has its been computed before for this row
                                    if (prev_level_ref == M_GRAM_LEVEL_UNDEF) {
                                        //If there has not been anything computed yet,
                                        //then first initialize the starting word
                                        hash_row_ref[begin_word_idx] = BASE::operator [](begin_word_idx);

                                        LOG_DEBUG1 << "word[" << SSTR(begin_word_idx) << "] = "
                                                << BASE::operator [](begin_word_idx)
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
                                        hash_row_ref[begin_word_idx] = combine_phrase_uids(hash_row_ref[begin_word_idx - 1], BASE::operator [](begin_word_idx));

                                        LOG_DEBUG1 << "hash[" << SSTR(begin_word_idx) << "] = combine( word["
                                                << SSTR(begin_word_idx) << "] = " << BASE::operator [](begin_word_idx)
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
                             * Tokenise a given piece of text into a space separated list of text pieces.
                             * @param text the piece of text to tokenise
                             * @param gram the gram container to put data into
                             */
                            inline void set_m_gram(const phrase_length num_words, const word_uid * word_ids) {
                                //Set all the "computed hash level" flags to "undefined"
                                memset(m_hash_level_row, M_GRAM_LEVEL_UNDEF, QUERY_M_GRAM_MAX_LEN * sizeof (phrase_length));

                                //Set the word ids into the parent
                                BASE::set_word_ids(num_words, word_ids);
                            }

                        private:
                            //Stores the hash computed flags
                            phrase_length m_hash_level_row[QUERY_M_GRAM_MAX_LEN];
                            //Stores the computed hash values
                            uint64_t m_hash_matrix[QUERY_M_GRAM_MAX_LEN][QUERY_M_GRAM_MAX_LEN];

                            /**
                             * This constructor is made private as it is not to be used
                             */
                            query_m_gram(word_uid * word_ids, phrase_length actual_level)
                            : phrase_base<QUERY_M_GRAM_MAX_LEN, LM_M_GRAM_LEVEL_MAX>(word_ids, actual_level) {
                            }

                            friend ostream& operator<<(ostream& stream, const query_m_gram & gram);
                        };
                    }
                }
            }
        }
    }
}

#endif /* QUERYMGRAM_HPP */

