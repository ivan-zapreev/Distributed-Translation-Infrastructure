/* 
 * File:   ModelMGram.hpp
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

#ifndef MODELMGRAM_HPP
#define MODELMGRAM_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/mgrams/m_gram_payload.hpp"

#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/math_utils.hpp"

#include "server/common/models/phrase_uid.hpp"

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        //Make a local constant to store the maximum number of words
                        static constexpr phrase_length MODEL_M_GRAM_MAX_LEN = LM_M_GRAM_LEVEL_MAX;

                        /**
                         * This class is used to represent the N-Gram that will be stored into the language model.
                         */
                        class model_m_gram : public phrase_base<MODEL_M_GRAM_MAX_LEN, MODEL_M_GRAM_MAX_LEN> {
                        public:
                            //Define the base class type
                            typedef phrase_base<MODEL_M_GRAM_MAX_LEN, MODEL_M_GRAM_MAX_LEN> BASE;

                            //Stores the m-gram payload i.e. its probability and back-off weight
                            m_gram_payload m_payload;

                            /**
                             * The basic constructor, is to be used when the M-gram level
                             * is known beforehand. Allows to set the actual M-gram level
                             * to a concrete value.
                             * @param actual_level the actual level of the m-gram that will be used should be <= MODEL_M_GRAM_MAX_LEN
                             */
                            model_m_gram(phrase_length actual_level)
                            : phrase_base<MODEL_M_GRAM_MAX_LEN, MODEL_M_GRAM_MAX_LEN>(m_word_ids, actual_level) {
                            }

                            /**
                             * Allows to start a new M-gram with the given level
                             * @param CURR_LEVEL the level of the M-gram we are starting
                             */
                            inline void start_new_m_gram() {
                                m_curr_index = BASE::get_first_word_idx();
                            }

                            /**
                             * Returns the reference to the next new token of the m-gram
                             * @return the reference to the next new token of the m-gram
                             */
                            inline text_piece_reader & get_next_new_token() {
                                ASSERT_SANITY_THROW((m_curr_index > BASE::get_last_word_idx()),
                                        string("The next token does not exist, exceeded the maximum of ") +
                                        to_string(BASE::get_num_words()) + string(" elements!"));

                                return m_tokens[m_curr_index++];
                            }

                            /**
                             * Allows to detect whether the given m-gram is an <unk> unigram.
                             * @return true if this is an <unk> unigram
                             */
                            inline bool is_unk_unigram() const {
                                return ((BASE::get_num_words() == M_GRAM_LEVEL_1) &&
                                        (m_tokens[BASE::get_first_word_idx()] == UNKNOWN_WORD_STR));
                            }

                            /**
                             * Allows to detect whether the given m-gram starts with an <s> tag.
                             * @return true starts with an <s> tag
                             */
                            inline bool is_sentence_begin() const {
                                return (m_tokens[BASE::get_first_word_idx()] == BEGIN_SENTENCE_TAG_STR);
                            }

                            /**
                             * Allows to prepare the M-gram for being used for adding it to the trie
                             * This includes registering the one gram in the word index
                             * @param word_index the word index to be used
                             */
                            template<typename WordIndexType>
                            inline void prepare_for_adding(WordIndexType & word_index) {
                                LOG_DEBUG1 << "Preparing the " << SSTR(BASE::get_num_words()) << "-gram for adding to the trie." << END_LOG;

                                //If we have a unigram then add it to the index otherwise get the word ids
                                if (BASE::get_num_words() == M_GRAM_LEVEL_1) {
                                    const phrase_length begin_word_idx = BASE::get_first_word_idx();
                                    if (word_index.is_word_registering_needed()) {
                                        //Register the word if it is needed
                                        m_word_ids[begin_word_idx] = word_index.register_word(m_tokens[begin_word_idx]);
                                    } else {
                                        //Otherwise jut get its id
                                        m_word_ids[begin_word_idx] = word_index.get_word_id(m_tokens[begin_word_idx]);
                                    }
                                    //The Unigram's hash value is equal to the word id
                                    m_hash_values[begin_word_idx] = m_word_ids[begin_word_idx];

                                    LOG_DEBUG1 << "word[" << SSTR(begin_word_idx) << "] = "
                                            << m_word_ids[begin_word_idx]
                                            << ", hash[" << SSTR(begin_word_idx) << "] = "
                                            << m_hash_values[begin_word_idx] << END_LOG;
                                } else {
                                    phrase_length curr_idx = BASE::get_first_word_idx();
                                    //Start with the first word
                                    m_word_ids[curr_idx] = word_index.get_word_id(m_tokens[curr_idx]);
                                    //The Unigram's hash value is equal to the word id
                                    m_hash_values[curr_idx] = m_word_ids[curr_idx];

                                    //Store the word ids without the unknown word flags and pre-compute the m-gram hash values
                                    for (++curr_idx; curr_idx <= BASE::get_last_word_idx(); ++curr_idx) {
                                        //Get the next word id
                                        m_word_ids[curr_idx] = word_index.get_word_id(m_tokens[curr_idx]);
                                        //Compute the next hash value 
                                        m_hash_values[curr_idx] = combine_phrase_uids(m_hash_values[curr_idx - 1], m_word_ids[curr_idx]);

                                        LOG_DEBUG1 << "hash[" << SSTR(curr_idx) << "] = combine( word["
                                                << SSTR(curr_idx) << "] = " << m_word_ids[curr_idx]
                                                << ", hash[" << SSTR(curr_idx - 1) << "] = "
                                                << m_hash_values[curr_idx - 1] << " ) = "
                                                << m_hash_values[curr_idx] << END_LOG;
                                    }
                                }
                            }

                            /**
                             * Allows to retrieve the hash value for the given m-gram
                             * @return the hash value for the given m-gram
                             */
                            inline uint64_t get_hash() const {
                                //The hash value is computed incrementally and backwards and therefore
                                //the full hash is stored under the index of the first n-gram's word
                                return m_hash_values[BASE::get_last_word_idx()];
                            }

                        private:
                            //Stores the word ids
                            word_uid m_word_ids[MODEL_M_GRAM_MAX_LEN] = {};

                            //Stores the m-gram tokens
                            text_piece_reader m_tokens[MODEL_M_GRAM_MAX_LEN] = {};

                            //The data structure to store the N-gram hashes
                            uint64_t m_hash_values[MODEL_M_GRAM_MAX_LEN] = {};

                            //Stores the m-gram idx for when adding m-gram tokens
                            phrase_length m_curr_index;
                            
                            friend ostream& operator<<(ostream& stream, const model_m_gram & gram);
                        };

                    }
                }
            }
        }
    }
}

#endif /* MODELMGRAM_HPP */

