/* 
 * File:   WordIndexTrie.hpp
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
 * Created on September 20, 2015, 5:24 PM
 */

#ifndef WORDINDEXTRIE_HPP
#define WORDINDEXTRIE_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/utils/file/text_piece_reader.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This is a common base class for all Trie implementations.
                     * The purpose of having this as a template class is performance optimization.
                     */
                    template<typename WordIndex>
                    class word_index_trie_base {
                    public:
                        typedef WordIndex WordIndexType;

                        /**
                         * The basic constructor
                         * @param word_index the word index to be used
                         */
                        explicit word_index_trie_base(WordIndexType & word_index)
                        : m_word_index(word_index) {
                        }

                        /**
                         * This method can be used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                         */
                        inline void pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                            m_word_index.reserve(counts[0]);
                            logger::update_progress_bar();
                        };

                        /**
                         * This method allows to check if post processing should be called after
                         * all the X level grams are read. This method is virtual.
                         * @tparam level the level of the X-grams that were finished to be read
                         */
                        template<phrase_length level>
                        inline bool is_post_grams() const {
                            return false;
                        }

                        /**
                         * This method should be called after all the X level grams are read.
                         * @tparam level the level of the X-grams that were finished to be read
                         */
                        template<phrase_length level>
                        inline void post_grams() {
                            THROW_MUST_OVERRIDE();
                        };

                        /**
                         * Allows to set the default UNK word probability value, the back-off is set to zero
                         * @param prob the unk word default probability value
                         */
                        void set_def_unk_word_prob(const prob_weight prob) {
                            THROW_MUST_OVERRIDE();
                        }

                        /**
                         * Allows to retrieve the stored word index, if any
                         * @return the pointer to the stored word index or NULL if none
                         */
                        inline WordIndexType & get_word_index() const {
                            return m_word_index;
                        };

                    protected:
                        //Stores the reference to the word index to be used
                        WordIndexType & m_word_index;
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
                    template class word_index_trie_base<basic_word_index >;
                    template class word_index_trie_base<counting_word_index>;
                    template class word_index_trie_base<optimizing_word_index<basic_word_index> >;
                    template class word_index_trie_base<optimizing_word_index<counting_word_index> >;
                }
            }
        }
    }
}

#endif /* WORDINDEXTRIE_HPP */

