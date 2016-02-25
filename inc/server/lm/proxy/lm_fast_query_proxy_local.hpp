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
                            : m_trie(trie), m_word_idx(m_trie.get_word_index()), m_query() {
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

                                //ToDo: Re-implement, all the computations are now as for a cumulative query!
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * @see lm_query_proxy
                             */
                            virtual phrase_length execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length min_level,
                                    prob_weight & prob) {

                                //ToDo: Re-implement, all the computations are now as for a cumulative query!
                                THROW_NOT_IMPLEMENTED();
                            }

                        protected:

                            /*
                             {
                                //Check that the given minimum level has a valid value
                                ASSERT_SANITY_THROW((min_level > LM_M_GRAM_LEVEL_MAX),
                                        string("An improper minimum level: ") + to_string(min_level) +
                                        string(" the maximum allowed level is: ") + to_string(LM_M_GRAM_LEVEL_MAX));

                                //Get the the flag value
                                constexpr bool is_ctx = trie_type::is_context_needed();

                                //Set the query data into the query object
                                m_query.set_data<is_ctx>(num_words, word_ids);

                                //Declare the result probability and initialize it with zero
                                prob_weight result = 0.0;

                                //Initialize the begin and end word indexes
                                phrase_length begin_word_idx = m_query.get_begin_word_idx();
                                phrase_length end_word_idx = begin_word_idx + min_level - 1;

                                //Iterate, there will be at least one iteration
                                do {
                                    //Set the begin and end word index to consider
                                    m_query.set_begin_end_word_idx(begin_word_idx, end_word_idx);

                                    //Execute the query
                                    m_trie.execute(m_query);

                                    prob += m_query.get_prob();

                                    //Increment the level but keep the maximum
                                    min_level = std::min<phrase_length>(min_level + 1, LM_M_GRAM_LEVEL_MAX);

                                } while (end_word_idx <= m_query.get_end_word_idx());

                                //Return the result
                                return min_level;
                             }
                             */

                        private:
                            //Stores the reference to the trie
                            const trie_type & m_trie;

                            //Stores the reference to the word index
                            const word_index_type & m_word_idx;

                            //Stores the reference to the sliding query
                            m_gram_query m_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_PROXY_IMPL_HPP */

