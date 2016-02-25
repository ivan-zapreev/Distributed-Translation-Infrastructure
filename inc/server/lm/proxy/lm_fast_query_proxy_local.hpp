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
                             * @param trie the trie to query
                             */
                            lm_fast_query_proxy_local(const trie_type & trie)
                            : m_trie(trie), m_word_idx(m_trie.get_word_index()), m_query(), m_joint_prob(0.0) {
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
                                return m_trie.get_unk_word_prob();
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual void get_word_ids(TextPieceReader phrase, phrase_length & num_words,
                                    word_uid word_ids[tm::TM_MAX_TARGET_PHRASE_LEN]) const {
                                //Initialize with zero words
                                num_words = 0;

                                //Declare the text piece reader for storing words
                                TextPieceReader word;

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

                                //return the probability value
                                return execute(num_words, word_ids, min_level);
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual phrase_length execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length min_level,
                                    prob_weight & prob) {

                                //Call the generic method, note that min_level is changed
                                prob = execute(num_words, word_ids, min_level);

                                //Return the last considered min level
                                return min_level;
                            }

                        protected:

                            /**
                             * Implements the query execution
                             * @param num_words the number of words in the query
                             * @param word_ids the word ids of the query
                             * @param min_level [in/out] the m-gram level to begin with/the maximum considered m-gram level
                             * @return the probability value
                             */
                            inline prob_weight execute(const phrase_length num_words, const word_uid * word_ids,
                                    phrase_length & min_level) {
                                //Re-initialize the joint prob reault with zero
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
                                get_interm_results(begin_word_idx, sub_end_word_idx, end_word_idx);

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
                                    get_interm_results(begin_word_idx, end_word_idx, end_word_idx);
                                }

                                //Set the min level to the current maximum
                                min_level = max_m_gram_level;
                                
                                //Return the final result;
                                return m_joint_prob;
                            }

                        protected:

                            /**
                             * Allows add up the intermediate results of the loose sub-sub queries defined by the arguments
                             * @param begin_word_idx the sub query begin word index
                             * @param first_end_word_idx the first sub-sub query end word index
                             * @param last_end_word_idx the last sub-sub query end word index
                             */
                            void get_interm_results(
                                    const phrase_length begin_word_idx,
                                    const phrase_length first_end_word_idx,
                                    const phrase_length last_end_word_idx) {
                                //Print the intermediate results
                                for (phrase_length end_word_idx = first_end_word_idx; end_word_idx <= last_end_word_idx; ++end_word_idx) {
                                    //Add all the weights even if they are zero, this is what the model tells us!
                                    m_joint_prob += m_query.m_probs[end_word_idx];
                                }
                            }

                        private:

                            //Store the flag indicating whether the trie needs context ids
                            static constexpr bool m_is_ctx = trie_type::is_context_needed();
                            
                            //Stores the reference to the trie
                            const trie_type & m_trie;

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

