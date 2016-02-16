/* 
 * File:   lm_index_query_proxy.hpp
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
 * Created on February 16, 2016, 11:39 AM
 */

#ifndef LM_INDEX_QUERY_PROXY_HPP
#define LM_INDEX_QUERY_PROXY_HPP

#include "common/utils/file/text_piece_reader.hpp"

using namespace uva::utils::file;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a word index query proxy interface class, it allows to interact with the word index in a uniform way.
                         */
                        class lm_index_query_proxy {
                        public:

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~lm_index_query_proxy() {
                            };
                            
                            /**
                             * Allows to get an id of the given word
                             * @param word the word to get the id for
                             * @return the word id
                             */
                            virtual uint64_t get_word_id(const TextPieceReader & word ) const = 0;
                        };
                    }
                }
            }
        }
    }
}

#endif /* LM_INDEX_QUERY_PROXY_HPP */

