/* 
 * File:   lm_index_query_proxy_local.hpp
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
 * Created on February 16, 2016, 11:40 AM
 */

#ifndef LM_INDEX_QUERY_PROXY_LOCAL_HPP
#define LM_INDEX_QUERY_PROXY_LOCAL_HPP

#include "common/utils/file/text_piece_reader.hpp"

using namespace uva::utils::file;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace proxy {

                        /**
                         * This is a local implementation of the word index query proxy interface,
                         * it allows to interact with the word index in a uniform way.
                         */
                        template<typename word_index>
                        class lm_index_query_proxy_local : public lm_index_query_proxy {
                        public:
                            
                            /**
                             * The basic constructor
                             * @param index the reference to the word index object
                             */
                            lm_index_query_proxy_local(word_index & index) : m_index(index){
                            }
                            
                            /**
                             * The basic destructor
                             */
                            virtual ~lm_index_query_proxy_local(){
                                //Nothing to be done, no dynamically allocated resources
                            }
                            
                            /**
                             * Allows to get an id of the given word
                             * @param word the word to get the id for
                             * @return the word id
                             */
                            virtual uint64_t get_word_id(const TextPieceReader & word ) const {
                                return m_index.get_word_id(word);
                            }
                            
                            private:
                                //Stores the reference to the word index
                                word_index& m_index;
                        };
                    }
                }
            }
        }
    }
}

#endif /* LM_INDEX_QUERY_PROXY_LOCAL_HPP */

