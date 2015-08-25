/* 
 * File:   C2WHybridMemoryTrie.cpp
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
 * Created on August 25, 2015, 11:27 PM
 */
#include "C2WHybridMemoryTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            C2WHybridMemoryTrie<N>::C2WHybridMemoryTrie(AWordIndex * const p_word_index)
            : ATrie<N>(p_word_index,
            [&] (const TShortId wordId, const TLongId ctxId, const TModelLevel level) -> TLongId {

                return this->getContextId(wordId, ctxId, level); }) {

                //Initialize the array of counters
                memset(next_ctx_id, 0, NUM_IDX_COUNTERS * sizeof (TShortId));
            }

            template<TModelLevel N>
            void C2WHybridMemoryTrie<N>::preAllocate(const size_t counts[N]) {
                //01) Pre-allocate the word index
                ATrie<N>::getWordIndex()->reserve(counts[0]);
            }

            template<TModelLevel N>
            void C2WHybridMemoryTrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                //ToDo: Implement
                throw Exception("Not implemented: HybridMemoryTrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result)");
            }

            template<TModelLevel N>
            C2WHybridMemoryTrie<N>::~C2WHybridMemoryTrie() {
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class C2WHybridMemoryTrie<MAX_NGRAM_LEVEL>;
        }
    }
}

