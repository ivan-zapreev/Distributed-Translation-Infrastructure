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

#include "server/lm/mgrams/m_gram_id.hpp"
#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::common::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        /**
                         * This class is used to represent the N-Gram that will be stored into the language model.
                         */
                        template<typename WordIndexType>
                        class model_m_gram : public m_gram_base<WordIndexType> {
                        public:
                            //Define the corresponding M-gram id type
                            typedef m_gram_id::Byte_M_Gram_Id<word_uid> T_M_Gram_Id;
                            //Define the base class type
                            typedef m_gram_base<WordIndexType> BASE;

                            //Stores the m-gram payload i.e. its probability and back-off weight
                            m_gram_payload m_payload;

                            //Stores the m-gram probability, the log_10 probability of the N-Gram Must be a negative value
                            prob_weight m_prob;

                            //Stores the m-gram log_10 back-off weight (probability) of the N-gram can be 0 is the probability is not available
                            prob_weight m_back_off;

                            /**
                             * The basic constructor, is to be used when the M-gram level
                             * is known beforehand. Allows to set the actual M-gram level
                             * to a concrete value.
                             * @param word_index the used word index
                             * @param actual_level the actual level of the m-gram that will be used should be <= M_GRAM_LEVEL_MAX
                             */
                            model_m_gram(WordIndexType & word_index, phrase_length actual_level)
                            : m_gram_base<WordIndexType>(word_index, actual_level) {
                            }

                            /**
                             * Allows to start a new M-gram with the given level
                             * @param CURR_LEVEL the level of the M-gram we are starting
                             */
                            inline void start_new_m_gram() {
                                m_curr_index = BASE::m_actual_begin_word_idx;
                            }

                            /**
                             * Returns the reference to the next new token of the m-gram
                             * @return the reference to the next new token of the m-gram
                             */
                            inline TextPieceReader & get_next_new_token() {
                                if (DO_SANITY_CHECKS && (m_curr_index > BASE::m_actual_end_word_idx)) {
                                    stringstream msg;
                                    msg << "The next token does not exist, exceeded the maximum of " << SSTR(BASE::m_actual_level) << " elements!";
                                    throw Exception(msg.str());
                                }
                                return BASE::m_tokens[m_curr_index++];
                            }

                            /**
                             * Allows to detect whether the given m-gram is an <unk> unigram.
                             * @return true if this is an <unk> unigram
                             */
                            inline bool is_unk_unigram() const {
                                return ((BASE::m_actual_level == M_GRAM_LEVEL_1) &&
                                        (BASE::m_tokens[BASE::m_actual_begin_word_idx] == UNKNOWN_WORD_STR));
                            }

                            /**
                             * Allows to prepare the M-gram for being used for adding it to the trie
                             * This includes registering the one gram in the word index
                             */
                            inline void prepare_for_adding() {
                                LOG_DEBUG1 << "Preparing the " << SSTR(BASE::m_actual_level) << "-gram for adding to the trie." << END_LOG;

                                //If we have a unigram then add it to the index otherwise get the word ids
                                if (BASE::m_actual_level == M_GRAM_LEVEL_1) {
                                    const phrase_length & begin_word_idx = BASE::m_actual_begin_word_idx;
                                    if (BASE::m_word_index.is_word_registering_needed()) {
                                        //Register the word if it is needed
                                        BASE::m_word_ids[begin_word_idx] = BASE::m_word_index.register_word(BASE::m_tokens[begin_word_idx]);
                                    } else {
                                        //Otherwise jut get its id
                                        BASE::m_word_ids[begin_word_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[begin_word_idx]);
                                    }
                                    //The Unigram's hash value is equal to the word id
                                    m_hash_values[begin_word_idx] = BASE::m_word_ids[begin_word_idx];

                                    LOG_DEBUG1 << "word[" << SSTR(begin_word_idx) << "] = "
                                            << BASE::m_word_ids[begin_word_idx]
                                            << ", hash[" << SSTR(begin_word_idx) << "] = "
                                            << m_hash_values[begin_word_idx] << END_LOG;
                                } else {
                                    phrase_length curr_idx = BASE::m_actual_begin_word_idx;
                                    //Start with the first word
                                    BASE::m_word_ids[curr_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[curr_idx]);
                                    //The Unigram's hash value is equal to the word id
                                    m_hash_values[curr_idx] = BASE::m_word_ids[curr_idx];

                                    //Store the word ids without the unknown word flags and pre-compute the m-gram hash values
                                    for (++curr_idx; curr_idx <= BASE::m_actual_end_word_idx; ++curr_idx) {
                                        //Get the next word id
                                        BASE::m_word_ids[curr_idx] = BASE::m_word_index.get_word_id(BASE::m_tokens[curr_idx]);
                                        //Compute the next hash value 
                                        m_hash_values[curr_idx] = combine_phrase_uids(m_hash_values[curr_idx - 1], BASE::m_word_ids[curr_idx]);

                                        LOG_DEBUG1 << "hash[" << SSTR(curr_idx) << "] = combine( word["
                                                << SSTR(curr_idx) << "] = " << BASE::m_word_ids[curr_idx]
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
                                return m_hash_values[BASE::m_actual_end_word_idx];
                            }

                        private:
                            //The data structure to store the N-gram hashes
                            uint64_t m_hash_values[LM_M_GRAM_LEVEL_MAX] = {};

                            //Stores the m-gram idx for when adding m-gram tokens
                            phrase_length m_curr_index;

                            /**
                             * Make this constructor private as it is not to be used.
                             */
                            model_m_gram(WordIndexType & word_index) : m_gram_base<WordIndexType>(word_index) {
                            }

                        };

                    }
                }
            }
        }
    }
}

#endif /* MODELMGRAM_HPP */

