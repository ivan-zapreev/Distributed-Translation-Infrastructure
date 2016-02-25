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

#ifndef LM_SLOW_QUERY_PROXY_LOCAL_HPP
#define LM_SLOW_QUERY_PROXY_LOCAL_HPP

#include <string>

#include "server/lm/proxy/lm_slow_query_proxy.hpp"
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
                        class lm_slow_query_proxy_local : public lm_slow_query_proxy {
                        public:
                            //Make a local typedef for the word index type
                            typedef typename trie_type::WordIndexType word_index_type;

                            /**
                             * The basic constructor that accepts the trie reference to query to
                             * @param trie the trie to query
                             */
                            lm_slow_query_proxy_local(const trie_type & trie)
                            : m_trie(trie), m_word_idx(m_trie.get_word_index()),
                            m_query(), m_num_words(0), m_joint_prob(0.0) {
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual ~lm_slow_query_proxy_local() {
                                //Nothing to free, all the resources are allocated on the stack.
                            }

                            /**
                             * Allows to execute m-gram the query. The query starts with the m-gram size
                             * given by min_level and then grows until the maximum of LM_M_GRAM_LEVEL_MAX.
                             * After that m-grams of the LM_M_GRAM_LEVEL_MAX are computed via a sliding window:
                             * Let:
                             *      "min_level == 2", "LM_MAX_QUERY_LEN = 4",
                             *      "num_word_ids == 6" and "word_ids == w1w2w3w4w5w6"
                             * Then this method will compute the sum:
                             *      P(w2|w1) + P(w3|w1w2) + P(w4|w1w2w3) + P(w5|w2w3w4) + P(w6|w3w4w5)
                             * @param line the text piece reader storing the m-gram query line
                             */
                            virtual void execute(TextPieceReader & line) {
                                //Re-initialize the joint prob reault with zero
                                m_joint_prob = 0.0;

                                //Parse the query line into tokens and get the word ids thereof
                                set_tokens_and_word_ids(line);

                                //Set the words data into the query object
                                m_query.set_data<m_is_ctx>(m_num_words, m_word_ids);

                                //Compute the maximum to consider m-gram level
                                const phrase_length max_m_gram_level = std::min<phrase_length>(m_num_words, LM_M_GRAM_LEVEL_MAX);

                                //Initialize the begin and end word indexes
                                phrase_length begin_word_idx = m_query.get_query_begin_word_idx();
                                phrase_length end_word_idx = begin_word_idx + max_m_gram_level - 1;

                                //Set the m-gram values for the first query execution
                                m_query.set_word_indxes(begin_word_idx, begin_word_idx, end_word_idx);

                                //Execute the first part of the query
                                m_trie.execute(m_query);

                                //Report the partial results, and update the total
                                get_report_interm_results(begin_word_idx, begin_word_idx, end_word_idx);

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
                            }

                        protected:

                            /**
                             * Allows report the intermediate results of the loose sub-sub queries defined by the arguments
                             * @param begin_word_idx the sub query begin word index
                             * @param first_end_word_idx the first sub-sub query end word index
                             * @param last_end_word_idx the last sub-sub query end word index
                             */
                            void get_report_interm_results(
                                    const phrase_length begin_word_idx,
                                    const phrase_length first_end_word_idx,
                                    const phrase_length last_end_word_idx) {
                                //Print the intermediate results
                                for (phrase_length end_word_idx = first_end_word_idx; end_word_idx <= last_end_word_idx; ++end_word_idx) {
                                    const string gram_str = get_m_gram_str(begin_word_idx, end_word_idx);

                                    LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                            << " ) ) = " << SSTR(m_query.m_probs[end_word_idx]) << END_LOG;
                                    LOG_INFO << "  Prob( " << gram_str << " ) = "
                                            << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_query.m_probs[end_word_idx])) << END_LOG;

                                    if (m_query.m_probs[end_word_idx] > ZERO_LOG_PROB_WEIGHT) {
                                        m_joint_prob += m_query.m_probs[end_word_idx];
                                    }
                                }
                            }

                            /**
                             * Allows to report the total joint probability of the query
                             */
                            void report_final_result() {
                                LOG_RESULT << "---" << END_LOG;
                                //Print the total cumulative probability if needed
                                const string gram_str = get_query_str();
                                LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                        << " ) ) = " << SSTR(m_joint_prob) << END_LOG;
                                LOG_INFO << "  Prob( " << gram_str << " ) = "
                                        << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_joint_prob)) << END_LOG;

                                LOG_RESULT << "-------------------------------------------" << END_LOG;
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
                                        const TextPieceReader & token = m_tokens[begin_word_idx];
                                        return token.str().empty() ? "<empty>" : token.str();
                                    } else {
                                        string result = m_tokens[end_word_idx].str() + " |";
                                        for (phrase_length idx = begin_word_idx; idx != end_word_idx; ++idx) {
                                            result += string(" ") + m_tokens[idx].str();
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
                                if (m_num_words == 0) {
                                    return "<none>";
                                } else {
                                    if (m_num_words == 1) {
                                        const TextPieceReader & token = m_tokens[0];
                                        return token.str().empty() ? "<empty>" : token.str();
                                    } else {
                                        string result;
                                        for (phrase_length idx = 0; idx < m_num_words; ++idx) {
                                            result += m_tokens[idx].str() + string(" ");
                                        }
                                        return result.substr(0, result.length() - 1);
                                    }
                                }
                            }

                            /**
                             * Allows to parse the m-gram into the tokens and get the word ids
                             */
                            virtual void set_tokens_and_word_ids(TextPieceReader phrase) {
                                //Initialize with zero words
                                m_num_words = 0;

                                LOG_DEBUG1 << "Getting word uids for phrase: ___" << phrase << "___" << END_LOG;

                                //Read the tokens one by one backwards and decrement the index
                                while (phrase.get_first_space(m_tokens[m_num_words])) {
                                    //Check that we do not get too many words!
                                    ASSERT_SANITY_THROW((m_num_words >= LM_MAX_QUERY_LEN),
                                            string("The m-gram query: ___") + phrase.str() +
                                            string("___ is too long, the maximum length is: ") +
                                            to_string(LM_MAX_QUERY_LEN));

                                    //Obtain the word id from the word index
                                    m_word_ids[m_num_words] = m_word_idx.get_word_id(m_tokens[m_num_words]);

                                    LOG_DEBUG1 << "Word: ___" << m_tokens[m_num_words]
                                            << "___ uid: " << m_word_ids[m_num_words] << END_LOG;

                                    //Increase the number of read words
                                    m_num_words++;
                                }
                            };

                        private:
                            //Stores the reference to the trie
                            const trie_type & m_trie;

                            //Stores the reference to the word index
                            const word_index_type & m_word_idx;

                            //Stores the reference to the sliding query
                            m_gram_query m_query;

                            //Stores the current m-gram query length
                            phrase_length m_num_words;

                            //Store the flag indicating whether the trie needs context ids
                            static constexpr bool m_is_ctx = trie_type::is_context_needed();

                            //Stores the m-gram word id array
                            word_uid m_word_ids[LM_MAX_QUERY_LEN] = {};

                            //Stores the m-gram tokens
                            TextPieceReader m_tokens[LM_MAX_QUERY_LEN] = {};

                            //Stores the joint probability result for the query
                            prob_weight m_joint_prob;
                        };

                        template<typename trie_type>
                        constexpr bool lm_slow_query_proxy_local<trie_type>::m_is_ctx;
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_PROXY_IMPL_HPP */

