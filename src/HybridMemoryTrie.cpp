/* 
 * File:   HybridMemoryTrie.cpp
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
 * Created on August 21, 2015, 4:18 PM
 */
#include "HybridMemoryTrie.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, class C>
            HybridMemoryTrie<N, C>::HybridMemoryTrie(AWordIndex * const p_word_index)
            : ATrie<N>(), m_p_word_index(p_word_index), m_mgram_data(NULL), m_mgram_mapping(NULL) {
                const size_t float_size = sizeof(TLogProbBackOff);
                const size_t idx_size = sizeof(TIndexSize);
                if( float_size != idx_size ) {
                    stringstream msg;
                    msg << "Unable to use " << __FILE__ << " for a trie as it expects ( sizeof(TLogProbBackOff) = "
                            << float_size << " ) == ( sizeof(TIndexSize) = " << idx_size << ")!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N, class C>
            void HybridMemoryTrie<N, C>::preAllocate(const size_t counts[N]) {
                //ToDo: Implement
            }

        }
    }
}
