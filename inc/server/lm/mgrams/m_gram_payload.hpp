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

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"

#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/math_utils.hpp"

#include "server/lm/mgrams/m_gram_id.hpp"
#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace uva::smt::bpbd::server::lm::m_grams::m_gram_id;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace m_grams {

                        /**
                         * This data structure stores the probability and back off weight payload for an m-gram
                         */
                        struct m_gram_payload_s {
                            TLogProbBackOff m_prob; // 4 byte for a float
                            TLogProbBackOff m_back; // 4 byte for a float

                            m_gram_payload_s() {
                            }

                            m_gram_payload_s(TLogProbBackOff prob, TLogProbBackOff back) {
                                m_prob = prob;
                                m_back = back;
                            }

                            operator string() const {
                                stringstream strbf;
                                strbf << "[ prob: " << m_prob << ", back: " << m_back << " ]";
                                return strbf.str();
                            }
                        };
                        typedef m_gram_payload_s m_gram_payload;

                        /**
                         * This class is the base class for all the M-gram classes used
                         */
                        template<typename WordIndexType>
                        class m_gram_base {
                        public:
                            //The type of the word id
                            typedef typename WordIndexType::TWordIdType TWordIdType;

                            //Declare the shorthand for the m-gram id type
                            typedef Byte_M_Gram_Id<TWordIdType> TM_Gram_Id;

                            //Define the corresponding M-gram id type
                            typedef m_gram_id::Byte_M_Gram_Id<TWordIdType> T_M_Gram_Id;

                            /**
                             * The basic constructor, is to be used when the M-gram level
                             * is known beforehand. Allows to set the actual M-gram level
                             * to a concrete value.
                             * @param word_index the used word index
                             * @param actual_level the actual level of the m-gram that will be used should be <= M_GRAM_LEVEL_MAX
                             */
                            m_gram_base(WordIndexType & word_index, TModelLevel actual_level)
                            : m_word_index(word_index), m_actual_level(actual_level),
                            m_actual_end_word_idx(actual_level - 1) {
                                //Perform sanity check if needed 
                                ASSERT_SANITY_THROW((m_actual_level > LM_M_GRAM_LEVEL_MAX),
                                        string("The provided actual level: ") + std::to_string(m_actual_level) +
                                        string(" exceeds the maximum level capacity: ") +
                                        std::to_string(LM_M_GRAM_LEVEL_MAX) + string(" of the T_Base_M_Gram class!"));

                                //Initialize the m-gram id pointer
                                m_gram_id_ptr = &m_gram_id[0];
                            }

                            /**
                             * The basic constructor, is to be used when the M-gram will
                             * actual level is not known beforehand - used e.g. in the query
                             * m-gram sub-class. The actual m-gram level is set to be
                             * undefined. Filling in the M-gram tokens is done elsewhere.
                             * @param word_index the used word index
                             */
                            m_gram_base(WordIndexType & word_index)
                            : m_word_index(word_index), m_actual_level(M_GRAM_LEVEL_UNDEF),
                            m_actual_end_word_idx(M_GRAM_LEVEL_UNDEF) {
                                //Initialize the m-gram id pointer
                                m_gram_id_ptr = &m_gram_id[0];
                            }

                            /**
                             * Allows to get the word index used in this m-gram
                             * @return the word index
                             */
                            inline WordIndexType & get_word_index() const {

                                return m_word_index;
                            }

                            /**
                             * Allows to obtain the actual m-gram level
                             * @return the actual m-gram level
                             */
                            inline TModelLevel get_m_gram_level() const {

                                return m_actual_level;
                            }

                            /**
                             * Allows to retrieve the actual end word id of the m-gram
                             * @return the id of the last word
                             */
                            inline TWordIdType get_end_word_id() const {

                                return m_word_ids[m_actual_end_word_idx];
                            }

                            /**
                             * Allows to retrieve the actual begin word index
                             * @return the index of the begin word
                             */
                            inline TModelLevel get_begin_word_idx() const {

                                return m_actual_begin_word_idx;
                            }

                            /**
                             * Allows to retrieve the actual end word index
                             * @return the index of the end word
                             */
                            inline TModelLevel get_end_word_idx() const {

                                return m_actual_end_word_idx;
                            }

                            /**
                             * Allows to get the address of the given word id cell by word index (number)
                             * Note the data array storing word ids is continuous.
                             * @param WORD_IDX the word index, the word number as in the m-gram w1w2w3w4w5
                             * @return the pointer to the cell storing the word id with the given index
                             */
                            template<TModelLevel WORD_IDX>
                            inline const TWordIdType * get_word_id_ptr() const {

                                return &m_word_ids[WORD_IDX];
                            }

                            /**
                             * Allows to work with the list of ids as with the continuous array.
                             * This function retrieves the pointer to the last word id of the m-gram.
                             * @return the pointer to the first word id element, 
                             */
                            inline const TWordIdType * word_ids() const {

                                return m_word_ids;
                            }

                            /**
                             * The basic to string conversion operator for the m-gram
                             */
                            inline operator string() const {

                                return tokens_to_string<LM_M_GRAM_LEVEL_MAX>(m_tokens, m_actual_begin_word_idx, m_actual_end_word_idx);
                            };

                            /**
                             * Allows to retrieve the word id for the given word index
                             * @param word_idx the word index
                             * @return the resulting word id
                             */
                            inline TWordIdType operator[](const TModelLevel word_idx) const {

                                return m_word_ids[word_idx];
                            };

                            /**
                             * Allows to create a new m-gram id for the sub-m-gram defined by the given of the method template parameters.
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
                            inline uint8_t create_m_gram_id(const TModelLevel begin_word_idx, const TModelLevel number_of_words, TM_Gram_Id_Value_Ptr & p_m_gram_id) const {
                                LOG_DEBUG << "Computing sub " << SSTR(number_of_words) << "-gram id for the gram "
                                        << "defined by the first word indexes: " << SSTR(begin_word_idx) << END_LOG;

                                //Create the M-gram id from the word ids.
                                uint8_t len_bytes = T_M_Gram_Id::create_m_gram_id(&m_word_ids[begin_word_idx], number_of_words, p_m_gram_id);

                                //Log the result
                                LOG_DEBUG << "Allocated " << number_of_words << "-gram id is: " << (void*) p_m_gram_id
                                        << ", with the byte length: " << SSTR(len_bytes) << END_LOG;

                                return len_bytes;
                            }

                            /**
                             * Allows to create a new m-gram id for the sub-m-gram defined by the given of the method template parameters.
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
                            inline const TM_Gram_Id_Value_Ptr get_m_gram_id_ref(const TModelLevel begin_word_idx, const TModelLevel number_of_words, uint8_t & len_bytes) {
                                LOG_DEBUG << "Computing sub " << SSTR(number_of_words) << "-gram id for the gram "
                                        << "defined by the first word indexes: " << SSTR(begin_word_idx) << END_LOG;

                                //Create the M-gram id from the word ids.
                                len_bytes = T_M_Gram_Id::compute_m_gram_id(&m_word_ids[begin_word_idx], number_of_words, m_gram_id_ptr);

                                //Log the result
                                LOG_DEBUG << "Initialized a new " << number_of_words << "-gram id of byte length: " << SSTR(len_bytes) << END_LOG;

                                return m_gram_id_ptr;
                            }

                        protected:
                            //Stores the m-gram tokens
                            TextPieceReader m_tokens[LM_M_GRAM_LEVEL_MAX];

                            //The data structure to store the N-gram word ids
                            TWordIdType m_word_ids[LM_M_GRAM_LEVEL_MAX] = {};

                            //Stores the reference to the used word index
                            WordIndexType & m_word_index;

                            //Stores the actual m-gram level, the number of meaningful elements in the tokens, the value of m for the m-gram
                            uint32_t m_actual_level;

                            //Declare the m-gram id container
                            DECLARE_STACK_GRAM_ID(TM_Gram_Id, m_gram_id, LM_M_GRAM_LEVEL_MAX);
                            //Declare the m-gram id pointer
                            TM_Gram_Id_Value_Ptr m_gram_id_ptr;

                            //These variables store the actual begin and end word index
                            //for all the words of this M-gram stored in the internal arrays
                            constexpr static TModelLevel m_actual_begin_word_idx = 0;
                            TModelLevel m_actual_end_word_idx;
                        };

                        template<typename WordIndexType>
                        constexpr TModelLevel m_gram_base<WordIndexType>::m_actual_begin_word_idx;

                    }
                }
            }
        }
    }
}

#endif /* BASEMGRAM_HPP */

