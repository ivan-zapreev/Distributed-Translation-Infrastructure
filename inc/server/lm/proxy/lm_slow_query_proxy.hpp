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

#ifndef LM_SLOW_QUERY_PROXY_HPP
#define LM_SLOW_QUERY_PROXY_HPP

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
                         */
                        class lm_slow_query_proxy {
                        public:

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~lm_slow_query_proxy() {
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
                            virtual void execute(text_piece_reader & line) = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRIE_QUERY_HPP */

