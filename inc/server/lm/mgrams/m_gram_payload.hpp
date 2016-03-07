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
#define BASEMGRAM_HPP

#include <string>       // std::string
#include <ostream>      // std::ostream

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"

#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/math_utils.hpp"

#include "server/lm/mgrams/m_gram_id.hpp"
#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::m_grams::m_gram_id;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        //Forward declaration of the payload structure
                        struct m_gram_payload_s;
                        //typedef for the payload structure
                        typedef m_gram_payload_s m_gram_payload;
                        //Forward declaration of the output operator
                        static inline ostream & operator<<(ostream & stream, const m_gram_payload & payload);
                        
                        /**
                         * This data structure stores the probability and back off weight payload for an m-gram
                         */
                        struct m_gram_payload_s {
                            prob_weight m_prob; // 4 byte for a float
                            prob_weight m_back; // 4 byte for a float

                            explicit m_gram_payload_s()
                            : m_prob(UNKNOWN_LOG_PROB_WEIGHT), m_back(UNKNOWN_LOG_PROB_WEIGHT) {
                                LOG_DEBUG2 << "Created " << *this << END_LOG;
                            }

                            m_gram_payload_s(prob_weight prob, prob_weight back)
                            : m_prob(prob),m_back(back) {
                                LOG_DEBUG2 << "Created " << *this << END_LOG;
                            }
                        };

                        static inline ostream & operator<<(ostream & stream, const m_gram_payload & payload) {
                            return stream << &payload << "=[ prob: " << payload.m_prob << ", back: " << payload.m_back << " ]";
                        }

                        /**
                         * This class is the base class for all the M-gram classes used
                         */
                        template<phrase_length MAX_PHRASE_LENGTH, phrase_length MAX_PHRASE_ID_LENGTH>
                        class phrase_base {
                        public:
                            //Declare the shorthand for the m-gram id type
                            typedef Byte_M_Gram_Id<word_uid> m_gram_id_type;

                            /**
                             * The basic constructor, is to be used when the M-gram level
                             * is known beforehand. Allows to set the actual M-gram level
                             * to a concrete value.
                             * @param word_ids the pointer to the word ids array to store
                             * NOTE: this pointer must remain through out the lifetime of
                             * the object, unless re-set by the appropriate method
                             * @param actual_level the actual level of the m-gram that will be used should be <= M_GRAM_LENGTH
                             */
                            phrase_base(word_uid * word_ids, phrase_length actual_level)
                            : m_word_ids(word_ids), m_num_words(actual_level),
                            m_last_word_idx(actual_level - 1) {
                                //Perform sanity check if needed 
                                ASSERT_SANITY_THROW((m_num_words > MAX_PHRASE_LENGTH),
                                        string("The provided number of words: ") + std::to_string(m_num_words) +
                                        string(" exceeds the maximum capacity: ") +
                                        std::to_string(MAX_PHRASE_LENGTH) + string(" of the T_Base_M_Gram class!"));

                                //Initialize the m-gram id pointer
                                m_phrase_id_ptr = &m_phrase_id[0];
                            }

                            /**
                             * The basic constructor, is to be used when the phrase will
                             * actual level is not known beforehand - used e.g. in the query
                             * m-gram sub-class. The actual m-gram level is set to be
                             * undefined. Filling in the phrase tokens is done elsewhere.
                             * @param word_index the used word index
                             */
                            phrase_base()
                            : m_num_words(0), m_last_word_idx(0) {
                                //Initialize the m-gram id pointer
                                m_phrase_id_ptr = &m_phrase_id[0];
                            }

                            /**
                             * Allows to obtain the actual m-gram level
                             * @return the actual m-gram level
                             */
                            inline phrase_length get_num_words() const {
                                return m_num_words;
                            }

                            /**
                             * Allows to retrieve the actual end word id of the m-gram
                             * @return the id of the last word
                             */
                            inline word_uid get_last_word_id() const {
                                return m_word_ids[m_last_word_idx];
                            }

                            /**
                             * Allows to retrieve the actual begin word index
                             * @return the index of the begin word
                             */
                            inline phrase_length get_first_word_idx() const {
                                return m_first_word_idx;
                            }

                            /**
                             * Allows to retrieve the actual end word index
                             * @return the index of the end word
                             */
                            inline phrase_length get_last_word_idx() const {
                                return m_last_word_idx;
                            }

                            /**
                             * Allows to work with the list of ids as with the continuous array.
                             * This function retrieves the pointer to the last word id of the m-gram.
                             * @return the pointer to the first word id element, 
                             */
                            inline const word_uid * word_ids() const {
                                return m_word_ids;
                            }

                            /**
                             * Allows get the word id for the given word index
                             * @param word_idx the word index
                             * @return the word id
                             */
                            inline word_uid operator[](const phrase_length word_idx) const {
                                ASSERT_SANITY_THROW((word_idx >= MAX_PHRASE_LENGTH),
                                        string("The provided word index: ") + std::to_string(word_idx) +
                                        string(" exceeds the maximum: ") + std::to_string(MAX_PHRASE_LENGTH - 1));

                                return m_word_ids[word_idx];
                            };

                            /**
                             * Allows to create a new m-gram id for the sub-hrase defined by the given of the method template parameters.
                             * For the argument reference to the id data pointer the following holds:
                             * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                             * as needed to store the given id.
                             * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                             * @param begin_word_idx the index of the first word in the sub-m-gram, indexes start with 0
                             * @param number_of_words the number of sub-m-gram words
                             * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                             *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                             * @param p_m_gram_id the reference to the M-gram id data pointer to be initialized with the M-gram id data, must be pre-allocated
                             */
                            inline uint8_t create_phrase_id(const phrase_length begin_word_idx, const phrase_length number_of_words, TM_Gram_Id_Value_Ptr & p_m_gram_id) const {
                                LOG_DEBUG << "Computing sub " << SSTR(number_of_words) << "-gram id for the gram "
                                        << "defined by the first word indexes: " << SSTR(begin_word_idx) << END_LOG;

                                //Get the end word index for sanity check
                                phrase_length end_word_idx = begin_word_idx + number_of_words - 1;

                                ASSERT_SANITY_THROW((end_word_idx >= MAX_PHRASE_LENGTH),
                                        string("The requested m-gram end index: ") + std::to_string(end_word_idx) +
                                        string(" exceeds the maximum: ") + std::to_string(MAX_PHRASE_LENGTH - 1));

                                //Create the M-gram id from the word ids.
                                uint8_t len_bytes = m_gram_id_type::create_m_gram_id(&m_word_ids[begin_word_idx], number_of_words, p_m_gram_id);

                                //Log the result
                                LOG_DEBUG << "Allocated " << number_of_words << "-gram id is: " << (void*) p_m_gram_id
                                        << ", with the byte length: " << SSTR(len_bytes) << END_LOG;

                                return len_bytes;
                            }

                            /**
                             * Allows to create a new m-gram id for the sub-phrase defined by the given of the method template parameters.
                             * For the argument reference to the id data pointer the following holds:
                             * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                             * as needed to store the given id.
                             * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                             * @param begin_word_idx the index of the first word in the sub-m-gram, indexes start with 0
                             * @param number_of_words the number of sub-m-gram words
                             * @param word_ids the list of the word ids for the entire m-gram, where at least the m-gram word
                             *                 ids for the sub-m-gram defined by the template parameters are known and initialized. 
                             * @param p_m_gram_id the reference to the M-gram id data pointer to be initialized with the M-gram id data, must be pre-allocated
                             */
                            inline const TM_Gram_Id_Value_Ptr get_phrase_id_ref(const phrase_length begin_word_idx, const phrase_length number_of_words, uint8_t & len_bytes) {
                                LOG_DEBUG << "Computing sub " << SSTR(number_of_words) << "-gram id for the gram "
                                        << "defined by the first word indexes: " << SSTR(begin_word_idx) << END_LOG;

                                //Get the end word index for sanity check
                                phrase_length end_word_idx = begin_word_idx + number_of_words - 1;

                                ASSERT_SANITY_THROW((end_word_idx >= MAX_PHRASE_LENGTH),
                                        string("The requested m-gram end index: ") + std::to_string(end_word_idx) +
                                        string(" exceeds the maximum: ") + std::to_string(MAX_PHRASE_LENGTH - 1));

                                //Create the M-gram id from the word ids.
                                len_bytes = m_gram_id_type::compute_m_gram_id(&m_word_ids[begin_word_idx], number_of_words, m_phrase_id_ptr);

                                //Log the result
                                LOG_DEBUG << "Initialized a new " << number_of_words << "-gram id of byte length: " << SSTR(len_bytes) << END_LOG;

                                return m_phrase_id_ptr;
                            }

                        protected:

                            /**
                             * Allows to set the pointer to the word ids
                             * @param word_ids the pointer to the void ids
                             * @param num_words the number of words in the array
                             */
                            void set_word_ids(const phrase_length num_words, const word_uid * word_ids) {
                                m_word_ids = word_ids;
                                m_num_words = num_words;
                                m_last_word_idx = m_num_words - 1;

                                ASSERT_SANITY_THROW(((m_num_words < M_GRAM_LEVEL_1) ||
                                        (m_num_words > MAX_PHRASE_LENGTH)),
                                        string("A broken N-gram query, level: ") +
                                        to_string(m_num_words));
                            }

                        private:
                            //Stores the pointer to the word ids array
                            const word_uid * m_word_ids;

                            //Stores the actual m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                            phrase_length m_num_words;

                            //Declare the m-gram id container, note that we do not make an id for more than LM_M_GRAM_LEVEL_MAX elements!
                            DECLARE_STACK_GRAM_ID(m_gram_id_type, m_phrase_id, MAX_PHRASE_ID_LENGTH);
                            //Declare the phrase's m-gram id pointer
                            TM_Gram_Id_Value_Ptr m_phrase_id_ptr;

                            //These variables store the actual begin and end word index
                            //for all the words of this phrase stored in the internal arrays
                            constexpr static phrase_length m_first_word_idx = 0;
                            phrase_length m_last_word_idx;
                        };

                        template<phrase_length MAX_M_GRAM_LENGTH, phrase_length MAX_M_GRAM_ID_LENGTH>
                        constexpr phrase_length phrase_base<MAX_M_GRAM_LENGTH, MAX_M_GRAM_ID_LENGTH>::m_first_word_idx;
                    }
                }
            }
        }
    }
}

#endif /* BASEMGRAM_HPP */

