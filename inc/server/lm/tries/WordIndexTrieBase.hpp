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
#define	WORDINDEXTRIE_HPP

#include <string>       // std::string

#include "server/lm/TrieConstants.hpp"
#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"

#include "common/utils/file/TextPieceReader.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is a common base class for all Trie implementations.
             * The purpose of having this as a template class is performance optimization.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N, typename WordIndex>
            class WordIndexTrieBase {
            public:
                static const TModelLevel MAX_LEVEL;
                typedef WordIndex WordIndexType;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit WordIndexTrieBase(WordIndexType & word_index)
                : m_word_index(word_index) {
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                inline void pre_allocate(const size_t counts[N]) {
                    m_word_index.reserve(counts[0]);
                    Logger::update_progress_bar();
                };

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * @param level the level of the X-grams that were finished to be read
                 */
                template<TModelLevel level>
                inline bool is_post_grams() const {
                    return false;
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * @param level the level of the X-grams that were finished to be read
                 */
                template<TModelLevel level>
                inline void post_grams() {
                    THROW_MUST_OVERRIDE();
                };

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

            template<TModelLevel N, typename WordIndex>
            const TModelLevel WordIndexTrieBase<N, WordIndex>::MAX_LEVEL = N;

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class WordIndexTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class WordIndexTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template class WordIndexTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template class WordIndexTrieBase<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;
        }
    }
}

#endif	/* WORDINDEXTRIE_HPP */

