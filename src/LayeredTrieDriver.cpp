/* 
 * File:   ATrie.cpp
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
 * Created on August 25, 2015, 29:27 PM
 */
#include "LayeredTrieDriver.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"

using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, TYPE) \
            INSTANTIATE_TRIE_TEMPLATE_TYPE(LayeredTrieDriver,T##TRIE_NAME##TYPE)

#define INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(TRIE_NAME) \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Basic); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Count); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptBasic); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptCount);

            /**************************************************************************/
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2DMapTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2WArrayTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(W2CArrayTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(W2CHybridTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2DHybridTrie);
            /**************************************************************************/

        }
    }
}

