/* 
 * File:   sliding_m_gram_query.hpp
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
 * Created on February 23, 2016, 3:28 PM
 */

#ifndef SLIDING_M_GRAM_QUERY_HPP
#define SLIDING_M_GRAM_QUERY_HPP

#include <string>       //std::string
#include <ostream>
#include <algorithm>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/server_configs.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/mgrams/query_m_gram.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::math::bits;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::m_grams;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This structure stores the basic data required for a query execution.
                     * m_query - the m-gram query itself 
                     * m_payloads - the two dimensional array of the payloads 
                     * m_last_ctx_ids - stores the last context id computed for the given row of the sub-m-gram matrix
                     * m_probs - the array of probabilities 
                     * m_begin_word_idx - the currently considered begin word index
                     * m_end_word_idx - the currently considered end word index
                     */
                    class m_gram_query {
                    public:
                        
                        //Stores the m-gram probability for the m-gram ending in this word
                        //The length of the m-gram is defined by the number of words in the
                        //query but is limited by the maximum Language model level.
                        prob_weight m_probs[QUERY_M_GRAM_MAX_LEN];

                        //The currently considered m-gram's begin word index
                        phrase_length m_curr_begin_word_idx;
                        //The currently considered m-gram's end word index
                        phrase_length m_curr_end_word_idx;

                        /**
                         * The basic constructor that gets a reference to the word index
                         */
                        m_gram_query() {
                        }

                        /**
                         * Allows to set new data to the query
                         * @param num_words stores the number of word ids, the maximum number
                         * of words must be QUERY_LENGTH
                         * @param word_ids the word identifiers of the words of the target phrase
                         * to compute the probability for
                         */
                        template<bool is_need_ctx_ids>
                        inline void set_data(const phrase_length num_words, const word_uid * word_ids) {
                            //Check that the number of words has a valid value
                            ASSERT_SANITY_THROW((num_words > QUERY_M_GRAM_MAX_LEN),
                                    string("The number of words in the query: ") + to_string(num_words) +
                                    string(" exceeds the allowed maximum: ") + to_string(QUERY_M_GRAM_MAX_LEN));

                            //Clean the probability and payload data
                            memset(m_probs, 0, sizeof (prob_weight) * QUERY_M_GRAM_MAX_LEN);
                            memset(m_payloads, 0, sizeof (m_gram_payload) * QUERY_M_GRAM_MAX_LEN * QUERY_M_GRAM_MAX_LEN);

                            //Clean the contexts if needed
                            if (is_need_ctx_ids) {
                                memset(m_last_ctx_ids, UNDEFINED_WORD_ID, sizeof (TLongId) * QUERY_M_GRAM_MAX_LEN);
                            }
                            
                            //Store the values
                            m_gram.set_m_gram(num_words, word_ids);
                        }
                        
                        /**
                         * Allows to get the begin word index of the query
                         * @return the begin word index of the query
                         */
                        inline phrase_length get_query_begin_word_idx() const {
                            return m_gram.get_first_word_idx();
                        }
                        
                        /**
                         * Allows to get the end word index of the query
                         * @return the end word index of the query
                         */
                        inline phrase_length get_query_end_word_idx() const {
                            return m_gram.get_last_word_idx();
                        }
                        
                        /**
                         * Allows to set the begin and end m-gram word index.
                         * These define the m-gram for which the probability is to be computed.
                         * This method is handy for when we need streaming for a number of
                         * sub-sub m-grams starting in the same word but of the incremented length
                         * @param sub_query_begin_word_idx the sub-query begin word index
                         * @param sub_sub_query_first_end_word_idx the sub-sub query first end word index
                         * @param sub_query_end_word_idx the sub query end word index
                         */
                        inline void set_word_indxes(
                        const phrase_length sub_query_begin_word_idx,
                        const phrase_length sub_sub_query_first_end_word_idx,
                        const phrase_length sub_query_end_word_idx) {
                            m_curr_begin_word_idx = sub_query_begin_word_idx;
                            m_curr_end_word_idx = sub_sub_query_first_end_word_idx;
                            m_sub_query_end_word_idx = sub_query_end_word_idx;
                        }
                        
                        /**
                         * Allows to set the begin and end m-gram word index.
                         * These define the m-gram for which the probability is to be computed.
                         * This method is needed for when we only need one m-gram probability without streaming
                         * @param sub_query_begin_word_idx the sub-query begin word index
                         * @param sub_query_end_word_idx the sub query end word index
                         */
                        inline void set_word_indxes(
                        const phrase_length sub_query_begin_word_idx,
                        const phrase_length sub_query_end_word_idx) {
                            m_curr_begin_word_idx = sub_query_begin_word_idx;
                            m_curr_end_word_idx = sub_query_end_word_idx;
                            m_sub_query_end_word_idx = sub_query_end_word_idx;
                        }

                        /**
                         * Allows to retrieve the word id under the given index
                         * @param idx the index of the word we need an id for
                         * @return the word id
                         */
                        inline word_uid operator[](const phrase_length idx) const {
                            //Return the value
                            return m_gram[idx];
                        }

                        /**
                         * Allows to check if the current sub-query execution is over or not
                         * @return true if the sub-query execution is not finished yet
                         */
                        inline bool is_not_finished() const {
                            //While we have not exceeded the number of words
                            return (m_curr_end_word_idx <= m_sub_query_end_word_idx);
                        }

                        /**
                         * Allows to compute the hash value of the m-gram defined
                         * by the current begin and end word indexes
                         * @return the hash of the current m-gram
                         */
                        inline uint64_t get_curr_m_gram_hash() {
                            return m_gram.get_hash(m_curr_begin_word_idx, m_curr_end_word_idx);
                        }

                        /**
                         * Allows to get the current begin word id
                         * @return the current begin word id
                         */
                        inline word_uid get_curr_begin_word_id() const {
                            return m_gram[m_curr_begin_word_idx];
                        }

                        /**
                         * Allows to get the current end word id
                         * @return the current end word id
                         */
                        inline word_uid get_curr_end_word_id() const {
                            return m_gram[m_curr_end_word_idx];
                        }

                        /**
                         * Allows to get the word if of the current uni-gram
                         * This method shall only be called in case:
                         *      m_curr_begin_word_idx == m_curr_end_word_idx
                         * @return the word id of the current uni-gram
                         */
                        inline word_uid get_curr_uni_gram_word_id() const {
                            //Assert sanity, check that it is indeed a unigram case!
                            ASSERT_SANITY_THROW((m_curr_begin_word_idx != m_curr_end_word_idx),
                                    string("Note a uni-gram, begin word idx: ") + to_string(m_curr_begin_word_idx) +
                                    string(" end word idx: ") + to_string(m_curr_end_word_idx));

                            //Return the required value
                            return m_gram[m_curr_begin_word_idx];
                        }

                        /**
                         * Allows to set the payload of the current m-gram defined
                         * by the current begin and end word indexes
                         * @param payload the payload to be set
                         */
                        inline void set_curr_payload(const m_gram_payload & payload) {
                            m_payloads[m_curr_begin_word_idx][m_curr_end_word_idx] = payload;
                        }

                        /**
                         * Allows to set the probability payload of the current m-gram defined
                         * by the current begin and end word indexes. The back-off is not set
                         * and is not changed.
                         * @param prob the probability value to be set
                         */
                        inline void set_curr_payload(const prob_weight prob) {
                            m_payloads[m_curr_begin_word_idx][m_curr_end_word_idx].m_prob = prob;
                        }

                        /**
                         * Allows to get the payload of the current m-gram defined
                         * by the current begin and end word indexes
                         * @return the reference to the payload
                         */
                        inline const m_gram_payload & get_curr_payload_ref() {
                            return m_payloads[m_curr_begin_word_idx][m_curr_end_word_idx];
                        }

                        /**
                         * Allows to check if the current m-gram is a uni-gram
                         * @return true if the current m-gram is a uni-gram, otherwise false
                         */
                        inline bool is_curr_uni_gram() const {
                            return (m_curr_begin_word_idx == m_curr_end_word_idx);
                        }

                        /**
                         * Allows to get the level of the currently considered m-gram
                         * @return the level of the currently considered m-gram
                         */
                        inline phrase_length get_curr_level() const {
                            return CURR_LEVEL_MAP[m_curr_begin_word_idx][m_curr_end_word_idx];
                        }

                        /**
                         * Allows to get the "level - 1" of the currently considered m-gram
                         * @return the "level - 1" of the currently considered m-gram
                         */
                        inline phrase_length get_curr_level_m1() const {
                            return CURR_LEVEL_MIN_1_MAP[m_curr_begin_word_idx][m_curr_end_word_idx];
                        }

                        /**
                         * Allows to get the "level - 2" of the currently considered m-gram
                         * @return the "level - 2" of the currently considered m-gram
                         */
                        inline phrase_length get_curr_level_m2() const {
                            return CURR_LEVEL_MIN_2_MAP[m_curr_begin_word_idx][m_curr_end_word_idx];
                        }

                        /**
                         * Allows to create a new m-gram id for the current m-gram defined by the current begin and end word index values.
                         * For the argument reference to the id data pointer the following holds:
                         * a) If there was no memory allocated for the M-gram id then there will be allocated as much
                         * as needed to store the given id.
                         * b) If there was memory allocated then no re-allocation will be done, then it is assumed that enough memory was allocated
                         * @param len_bytes the length in bytes
                         * @return the M-gram id data pointer
                         */
                        inline const TM_Gram_Id_Value_Ptr get_curr_m_gram_id(uint8_t & len_bytes) {
                            return m_gram.get_phrase_id_ref(m_curr_begin_word_idx, get_curr_level(), len_bytes);
                        }

                        /**
                         * Allows to get a reference to the current context
                         * @return the reference to the variable storing the current context value
                         */
                        inline TLongId & get_curr_ctx_ref() {
                            return m_last_ctx_ids[m_curr_begin_word_idx];
                        }

                    protected:
                    private:
                        //Stores the query m-gram
                        query_m_gram m_gram;
                        
                        //Stores the current execution last word index
                        phrase_length m_sub_query_end_word_idx;

                        //Stores the retrieved payloads
                        m_gram_payload m_payloads[QUERY_M_GRAM_MAX_LEN][QUERY_M_GRAM_MAX_LEN];

                        //Stores the currently computed context for the last pair
                        //of begin and end word ids, only for layered tries.
                        TLongId m_last_ctx_ids[QUERY_M_GRAM_MAX_LEN];

                        //Add the stream operator as a friend
                        friend ostream& operator<<(ostream& stream, const m_gram_query & query);
                    };
                }
            }
        }
    }
}

#endif /* SLIDING_M_GRAM_QUERY_HPP */

