/* 
 * File:   HybridMemoryTrie.hpp
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

#ifndef HYBRIDMEMORYTRIE_HPP
#define	HYBRIDMEMORYTRIE_HPP

#include <string>

#include "Globals.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            class HybridMemoryTrie : public ATrie<N> {
            public:
                explicit HybridMemoryTrie(AWordIndex * const _pWordIndex);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * For more details @see ATrie
                 */
                virtual void add1Gram(const SRawNGram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * For more details @see ATrie
                 */
                virtual void addMGram(const SRawNGram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * For more details @see ATrie
                 */
                virtual void addNGram(const SRawNGram &nGram);

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * For more details @see ATrie
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result);

                /**
                 * The basic destructor
                 */
                virtual ~HybridMemoryTrie();

            private:

            };
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

