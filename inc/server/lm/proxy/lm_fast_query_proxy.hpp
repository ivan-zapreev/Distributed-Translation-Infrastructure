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

#ifndef LM_FAST_QUERY_PROXY_HPP
#define LM_FAST_QUERY_PROXY_HPP

#include <map>

#include "common/utils/file/text_piece_reader.hpp"

#include "server/server_configs.hpp"
#include "server/lm/lm_consts.hpp"

using namespace std;
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
                         */
                        class lm_fast_query_proxy {
                        public:

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~lm_fast_query_proxy() {
                            }

                            /**
                             * Allows to retrieve the unknown target word log probability penalty 
                             * @return the target source word log probability penalty
                             */
                            virtual prob_weight get_unk_word_prob() const = 0;

                            /**
                             * Allows to retrieve the begin tag uid value
                             * @return the begin tag "<s>" uid
                             */
                            virtual const word_uid & get_begin_tag_uid() const = 0;

                            /**
                             * Allows to retrieve the end tag uid value
                             * @return the end tag "</s>" uid
                             */
                            virtual const word_uid & get_end_tag_uid() const = 0;

                            /**
                             * Allows to retrieve the target language phrase word ids.
                             * Note that the number of words in the target phrase should
                             * not exceed:  TM_MAX_TARGET_PHRASE_LEN
                             * @param phrase [in] the target language phrase
                             * @param num_words [out] the number of words to be set
                             * @param word_ids [out] the words ids to be set
                             */
                            virtual void get_word_ids(text_piece_reader phrase, phrase_length & num_words,
                                    word_uid word_ids[tm::TM_MAX_TARGET_PHRASE_LEN]) const = 0;

                            /**
                             * Allows to execute m-gram the query. The query starts with the m-gram size
                             * one (1) and then grows until the maximum of LM_M_GRAM_LEVEL_MAX.
                             * After that m-grams of the LM_M_GRAM_LEVEL_MAX are computed via a sliding window:
                             * Let:
                             *      "LM_MAX_QUERY_LEN = 4",
                             *      "num_word_ids == 6" and "word_ids == w1w2w3w4w5w6"
                             * Then this method will compute the sum:
                             *      P(w1) + P(w2|w1) + P(w3|w1w2) + P(w4|w1w2w3) + P(w5|w2w3w4) + P(w6|w3w4w5)
                             * @param num_words stores the number of word ids, the maximum number
                             * of words must be LM_MAX_QUERY_LEN
                             * @param word_ids the word identifiers of the words of the target phrase
                             * to compute the probability for
                             * @return the probability weight for the given query
                             */
                            virtual prob_weight execute(const phrase_length num_words,
                                    const word_uid * word_ids) = 0;

                            /**
                             * Allows to execute m-gram the query. The query starts with the m-gram size
                             * given by min_level and then grows until the maximum of LM_M_GRAM_LEVEL_MAX.
                             * After that m-grams of the LM_M_GRAM_LEVEL_MAX are computed via a sliding window:
                             * Let:
                             *      "min_level == 2", "LM_MAX_QUERY_LEN = 4",
                             *      "num_word_ids == 6" and "word_ids == w1w2w3w4w5w6"
                             * Then this method will compute the sum:
                             *      P(w2|w1) + P(w3|w1w2) + P(w4|w1w2w3) + P(w5|w2w3w4) + P(w6|w3w4w5)
                             * @param [in] num_words stores the number of word ids, the maximum number
                             * of words must be LM_MAX_QUERY_LEN
                             * @param [in] word_ids the word identifiers of the words of the target phrase
                             * to compute the probability for
                             * @param [in/out] min_level the first m-gram level to consider, the next
                             * minimum m-gram level to consider, is limited by LM_M_GRAM_LEVEL_MAX
                             * @return the resulting probability weight
                             */
                            virtual prob_weight execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length & min_level) = 0;

                            /**
                             * Allows to execute m-gram the query. The query starts with the m-gram size
                             * given by min_level and then grows until the maximum of LM_M_GRAM_LEVEL_MAX.
                             * After that m-grams of the LM_M_GRAM_LEVEL_MAX are computed via a sliding window:
                             * Let:
                             *      "min_level == 2", "LM_MAX_QUERY_LEN = 4",
                             *      "num_word_ids == 6" and "word_ids == w1w2w3w4w5w6"
                             * Then this method will compute the sum:
                             *      P(w2|w1) + P(w3|w1w2) + P(w4|w1w2w3) + P(w5|w2w3w4) + P(w6|w3w4w5)
                             * @param [in] num_words stores the number of word ids, the maximum number
                             * of words must be LM_MAX_QUERY_LEN
                             * @param [in] word_ids the word identifiers of the words of the target phrase
                             * to compute the probability for
                             * @param [in/out] min_level the first m-gram level to consider, the next
                             * minimum m-gram level to consider, is limited by LM_M_GRAM_LEVEL_MAX
                             * @param scores the pointer to the array of feature scores that is to 
                             *               be filled in, unless the provided pointer is NULL.
                             * @return the resulting probability weight
                             */
                            virtual prob_weight execute(const phrase_length num_words,
                                    const word_uid * word_ids, phrase_length & min_level, 
                                    prob_weight * scores) = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_HPP */

