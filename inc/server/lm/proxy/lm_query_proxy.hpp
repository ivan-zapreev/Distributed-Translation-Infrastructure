/* 
 * File:   lm_trie_query_proxy.hpp
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
 * Created on February 5, 2016, 8:46 AM
 */

#ifndef LM_TRIE_QUERY_PROXY_HPP
#define LM_TRIE_QUERY_PROXY_HPP

#include "common/utils/file/text_piece_reader.hpp"

#include "server/server_configs.hpp"
#include "server/lm/lm_consts.hpp"

using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This class represents a trie query proxy interface class.
                         * It allows to interact with templated trie queries in a uniform way.
                         * ToDo: Add methods/return values to extract the needed probability data.
                         */
                        class lm_query_proxy {
                        public:

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~lm_query_proxy() {
                            }

                            /**
                             * Allows to retrieve the unknown target word log probability penalty 
                             * @return the target source word log probability penalty
                             */
                            virtual prob_weight get_unk_word_prob() = 0;

                            /**
                             * Allows to retrieve the target language phrase word ids.
                             * Note that the number of words in the target phrase should
                             * not exceed:  TM_MAX_TARGET_PHRASE_LEN
                             * @param phrase [in] the target language phrase
                             * @param num_words [out] the number of words to be set
                             * @param word_ids [out] the words ids to be set
                             */
                            virtual void get_word_ids(TextPieceReader phrase, phrase_length & num_words,
                                    word_uid word_ids[tm::TM_MAX_TARGET_PHRASE_LEN]) = 0;

                            /**
                             * Allows to execute m-gram the query. The query starts with the m-gram size
                             * given by min_level and then grows until the maximum of LM_M_GRAM_LEVEL_MAX.
                             * After that m-grams of the LM_M_GRAM_LEVEL_MAX are computed via a sliding window:
                             * Let:
                             *      "min_level == 2", "LM_MAX_QUERY_LEN = 4",
                             *      "num_word_ids == 6" and "word_ids == w1w2w3w4w5w6"
                             * Then this method will compute the sum:
                             *      P(w2|w1) + P(w3|w1w2) + P(w4|w1w2w3) + P(w5|w2w3w4) + P(w6|w3w4w5)
                             * @param min_level the minimum value of the m-gram level
                             * @param num_word_ids stores the number of word ids, the maximum number
                             * of words must be LM_MAX_QUERY_LEN
                             * @param word_ids the word identifiers of the words of the target phrase
                             * to compute the probability for
                             */
                            template<bool is_log_result = false >
                            inline prob_weight execute(const phrase_length min_level,
                                    const phrase_length num_word_ids, const word_uid * word_ids) {
                                if (is_log_result) {
                                    return execute_log_yes(min_level, num_word_ids, word_ids);
                                } else {
                                    return execute_log_no(min_level, num_word_ids, word_ids);
                                }
                            };

                            /**
                             * Allows to execute a query
                             * @param is_cumulative, if true then we compute the joint probability
                             * i.e. the sum of the probabilities of the sub-m-gram prefixes until
                             * the max m-gram level plus the sliding window.
                             * @param text the m-gram query to be executed
                             */
                            template<bool is_cumulative = false >
                            inline prob_weight execute(TextPieceReader &text) {
                                if (is_cumulative) {
                                    return execute_cum_yes(text);
                                } else {
                                    return execute_cum_no(text);
                                }
                            };

                        protected:

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual prob_weight execute_log_yes(const phrase_length min_level,
                                    const phrase_length num_word_ids, const word_uid * word_ids) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual prob_weight execute_log_no(const phrase_length min_level,
                                    const phrase_length num_word_ids, const word_uid * word_ids) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual prob_weight execute_cum_yes(TextPieceReader &text) = 0;

                            /**
                             * This function is to be implemented by the child and
                             * should allow for a specific type of query execution
                             * cumulative/single, with/without logging.
                             */
                            virtual prob_weight execute_cum_no(TextPieceReader &text) = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_HPP */

