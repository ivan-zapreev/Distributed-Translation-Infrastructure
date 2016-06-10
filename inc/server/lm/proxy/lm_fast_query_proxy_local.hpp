/* 
 * File:   lm_trie_query_proxy_local.hpp
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
 * Created on February 5, 2016, 8:47 AM
 */

#ifndef LM_FAST_QUERY_PROXY_LOCAL_HPP
#define LM_FAST_QUERY_PROXY_LOCAL_HPP

#include <algorithm>

#include "server/lm/lm_parameters.hpp"
#include "server/lm/proxy/lm_fast_query_proxy.hpp"
#include "server/lm/models/m_gram_query.hpp"

using namespace std;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a local implementation of the language model query
                         * This implementation works with the local trie
                         */
                        template<typename trie_type>
                        class lm_fast_query_proxy_local : public lm_fast_query_proxy {
                        public:
                            //Make a local typedef for the word index type
                            typedef typename trie_type::WordIndexType word_index_type;

                            /**
                             * The basic constructor that accepts the trie reference to query to
                             * Note that the begin and end tag uids are provided only for the sake of performance optimization.
                             * @param m_params the lm model parameters
                             * @param trie the trie to query
                             * @param unk_word_prob the unknown word LM probability
                             * @param begin_tag_uid the begin sentence tag word uid
                             * @param end_tag_uid the begin sentence tag word uid
                             */
                            lm_fast_query_proxy_local(const lm_parameters & params, const trie_type & trie,
                                    const prob_weight& unk_word_prob, const word_uid & begin_tag_uid,
                                    const word_uid & end_tag_uid)
                            : m_params(params), m_trie(trie), m_unk_word_prob(unk_word_prob),
                            m_begin_tag_uid(begin_tag_uid), m_end_tag_uid(end_tag_uid),
                            m_word_idx(m_trie.get_word_index()), m_query(), m_joint_prob(0.0) {
                            }

                            /**
                             * @see lm_fast_query_proxy
                             */
                            virtual ~lm_fast_query_proxy_local() {
                                //Nothing to free, all the resources are allocated on the stack.
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight get_unk_word_prob() const {
                                return m_unk_word_prob;
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual const word_uid & get_begin_tag_uid() const {
                                return m_begin_tag_uid;
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual const word_uid & get_end_tag_uid() const {
                                return m_end_tag_uid;
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual void get_word_ids(text_piece_reader phrase, phrase_length & num_words,
                                    word_uid word_ids[tm::TM_MAX_TARGET_PHRASE_LEN]) const {
                                //Initialize with zero words
                                num_words = 0;

                                //Declare the text piece reader for storing words
                                text_piece_reader word;

                                LOG_DEBUG1 << "Getting word uids for phrase: ___" << phrase << "___" << END_LOG;

                                //Read the tokens one by one backwards and decrement the index
                                while (phrase.get_first_space(word)) {
                                    //Check that we do not get too many words!
                                    ASSERT_SANITY_THROW((num_words >= tm::TM_MAX_TARGET_PHRASE_LEN),
                                            string("The target phrase: ___") + phrase.str() +
                                            string("___ has too many words, the allowed maximum is: ") +
                                            to_string(tm::TM_MAX_TARGET_PHRASE_LEN));

                                    //Obtain the word id from the word index
                                    word_ids[num_words] = m_word_idx.get_word_id(word);

                                    LOG_DEBUG1 << "Word: ___" << word << "___ has uid: " << word_ids[num_words] << END_LOG;

                                    //Increase the number of read words
                                    num_words++;
                                }
                            };

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute(const phrase_length num_words, const word_uid * word_ids) {
                                //Declare a dummy variable for the min level
                                phrase_length min_level = M_GRAM_LEVEL_1;

                                //Compute the probability value
                                prob_weight prob = execute_query<false>(num_words, word_ids, min_level);

                                LOG_DEBUG1 << "The resulting LM query probability is: " << prob << END_LOG;

                                //Return the probability result
                                return prob;
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length & min_level) {
                                return execute_query<false>(num_words, word_ids, min_level);
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual prob_weight execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length & min_level,
                                    prob_weight * scores) {
                                return execute_query(num_words, word_ids, min_level, scores);
                            }

                        protected:

                            template<bool is_consider_scores = true >
                            inline prob_weight execute_query(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length & min_level,
                                    prob_weight * scores = NULL) {
                                //Re-initialize the joint prob result with zero
                                m_joint_prob = 0.0;

                                //Set the words data into the query object
                                m_query.set_data<m_is_ctx>(num_words, word_ids);

                                //Compute the maximum to consider m-gram level
                                const phrase_length max_m_gram_level = std::min<phrase_length>(num_words, LM_M_GRAM_LEVEL_MAX);

                                //Initialize the begin and end word indexes
                                phrase_length begin_word_idx = 0;
                                phrase_length sub_end_word_idx = min_level - 1;
                                phrase_length end_word_idx = max_m_gram_level - 1;

                                //Check that the minimum level is actually possible!
                                ASSERT_SANITY_THROW((sub_end_word_idx > end_word_idx),
                                        string("Impossible min_level: ") + to_string(min_level) +
                                        string(" the maximum possible level is: ") + to_string(max_m_gram_level));

                                //Set the m-gram values for the first query execution
                                m_query.set_word_indxes(begin_word_idx, sub_end_word_idx, end_word_idx);

                                //Execute the first part of the query
                                m_trie.execute(m_query);

                                //Report the partial results, and update the total
                                get_report_interm_results(begin_word_idx, sub_end_word_idx, end_word_idx);

                                //Now do the sliding window and compute more probabilities,
                                //Note that if the end_word_idx is smaller than the query
                                //last word idx then it means that:
                                //      (max_m_gram_level == LM_M_GRAM_LEVEL_MAX)
                                //and there is still m-grams to compute
                                while (end_word_idx < m_query.get_query_end_word_idx()) {
                                    //Slide the window one step forward
                                    begin_word_idx++;
                                    end_word_idx++;

                                    //Set the window value inside, this time we need a single probability and not the joint
                                    m_query.set_word_indxes(begin_word_idx, end_word_idx);

                                    //Execute the query
                                    m_trie.execute(m_query);

                                    //Report the partial result, and update the total
                                    get_report_interm_results(begin_word_idx, end_word_idx, end_word_idx);
                                }

                                //Report the total result
                                report_final_result();

                                //Compute the next minimum level to consider, it is either one level higher or we are at the maximum
                                min_level = std::min<phrase_length>(max_m_gram_level + 1, LM_M_GRAM_LEVEL_MAX);

                                LOG_DEBUG << "Computed log10(Prob(" << m_query << ")) = " << m_joint_prob << ", next min_level:  " << min_level << END_LOG;

#if IS_SERVER_TUNING_MODE
                                //Report the feature scores, here we do it outside the model - for
                                //simplicity, also only in case that the scores map is present
                                if (is_consider_scores) {
                                    ASSERT_SANITY_THROW((scores == NULL), string("The scores pointer is NULL!"));
                                    //Store the score and divide it by the lambda weight to restore the original!
                                    const prob_weight pure_cost = m_joint_prob / m_params.get_0_lm_weight();
                                    scores[lm_parameters::LM_WEIGHT_GLOBAL_IDS[0]] = pure_cost;
                                    LOG_DEBUG2 << lm_parameters::LM_WEIGHT_NAMES[0] << " = " << pure_cost << END_LOG;
                                }
#endif

                                //Return the final result;
                                return m_joint_prob;
                            }

                            /**
                             * For the given N-gram, for some level M <=N , this method
                             * allows to give the string of the object for which the
                             * probability is computed, e.g.:
                             * N-gram = "word1" -> result = "word1"
                             * N-gram = "word1 word2 word3" -> result = "word3 | word1  word2"
                             * for the first M tokens of the N-gram
                             * @param begin_word_idx the m-gram's begin word index
                             * @param end_word_idx the m-gram's begin word index
                             * @return the resulting string
                             */
                            inline string get_m_gram_str(const phrase_length begin_word_idx, const phrase_length end_word_idx) const {
                                if (begin_word_idx > end_word_idx) {
                                    return "<none>";
                                } else {
                                    if (begin_word_idx == end_word_idx) {
                                        return to_string(m_query[begin_word_idx]);
                                    } else {
                                        string result = to_string(m_query[end_word_idx]) + " |";
                                        for (phrase_length idx = begin_word_idx; idx != end_word_idx; ++idx) {
                                            result += string(" ") + to_string(m_query[idx]);
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
                            inline string get_query_str() const {
                                const phrase_length begin_idx = m_query.get_query_begin_word_idx();
                                const phrase_length end_idx = m_query.get_query_end_word_idx();
                                if (begin_idx == end_idx) {
                                    return to_string(m_query[begin_idx]);
                                } else {
                                    string result;
                                    for (phrase_length idx = begin_idx; idx <= end_idx; ++idx) {
                                        result += to_string(m_query[idx]) + string(" ");
                                    }
                                    return result.substr(0, result.length() - 1);
                                }
                            }

                            /**
                             * Allows add up the intermediate results of the loose sub-sub queries defined by the arguments
                             * @param begin_word_idx the sub query begin word index
                             * @param first_end_word_idx the first sub-sub query end word index
                             * @param last_end_word_idx the last sub-sub query end word index
                             */
                            inline void get_report_interm_results(
                                    const phrase_length begin_word_idx,
                                    const phrase_length first_end_word_idx,
                                    const phrase_length last_end_word_idx) {
                                //Print the intermediate results
                                for (phrase_length end_word_idx = first_end_word_idx; end_word_idx <= last_end_word_idx; ++end_word_idx) {
                                    if (MAXIMUM_LOGGING_LEVEL >= debug_levels_enum::DEBUG) {
                                        const string gram_str = get_m_gram_str(begin_word_idx, end_word_idx);

                                        LOG_DEBUG << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                                << " ) ) = " << SSTR(m_query.m_probs[end_word_idx]) << END_LOG;
                                        LOG_DEBUG1 << "  Prob( " << gram_str << " ) = "
                                                << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_query.m_probs[end_word_idx])) << END_LOG;
                                    }

                                    //Do not add anything below the zero weight.
                                    if (m_query.m_probs[end_word_idx] >= ZERO_LOG_PROB_WEIGHT) {
                                        m_joint_prob += m_query.m_probs[end_word_idx];
                                    }
                                }
                            }

                            /**
                             * Allows to report the total joint probability of the query
                             */
                            inline void report_final_result() {
                                if (MAXIMUM_LOGGING_LEVEL >= debug_levels_enum::DEBUG) {
                                    LOG_DEBUG << "---" << END_LOG;
                                    //Print the total cumulative probability if needed
                                    const string gram_str = get_query_str();
                                    LOG_DEBUG << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                            << " ) ) = " << SSTR(m_joint_prob) << END_LOG;
                                    LOG_DEBUG1 << "  Prob( " << gram_str << " ) = "
                                            << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_joint_prob)) << END_LOG;
                                    LOG_DEBUG << "-------------------------------------------" << END_LOG;
                                }
                            }

                        private:

                            //Store the flag indicating whether the trie needs context ids
                            static constexpr bool m_is_ctx = trie_type::is_context_needed();

                            //Stores the pointer to the configuration parameters
                            const lm_parameters & m_params;

                            //Stores the reference to the trie
                            const trie_type & m_trie;

                            //Stores the cached unknown word probability from LM
                            const prob_weight m_unk_word_prob;

                            //Sore the cached begin and end sentence tag word uids as retrieved from the LM word index.
                            const word_uid m_begin_tag_uid;
                            const word_uid m_end_tag_uid;

                            //Stores the reference to the word index
                            const word_index_type & m_word_idx;

                            //Stores the reference to the sliding query
                            m_gram_query m_query;

                            //Stores the joint probability result for the query
                            prob_weight m_joint_prob;
                        };

                        template<typename trie_type>
                        constexpr bool lm_fast_query_proxy_local<trie_type>::m_is_ctx;
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_PROXY_IMPL_HPP */

