/* 
 * File:   TrieTypeFactory.hpp
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
 * Created on August 27, 2015, 21:42 PM
 */
#include <string>

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "ATrie.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

#ifndef TRIETYPEFACTORY_HPP
#define	TRIETYPEFACTORY_HPP

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is a factory clas for different sorts of tries
             * ToDo: Improve, make decent and better, this is a very crude implementation of a factory
             */
            class TrieTypeFactory {
            private:
                static const string TC2DHashMapTrie_STR;
                static const string TW2CHybridMemoryTrie_STR;
                static const string TC2WOrderedArrayTrie_STR;
                static const string TW2COrderedArrayTrie_STR;
                static const string C2DMapArrayTrie_STR;
                static const string G2DHashMapTrie_STR;

            public:

                /**
                 * Instantiate a trie for the given string name
                 * @param trie_type the trie type name
                 * @return the instance of trie
                 * @throws Exception if the trie type name is not recognized
                 */
                template<TModelLevel N>
                static inline ATrie<N>* getTrie(const string trie_type) {
                    const size_t memory_factor = __HashMapWordIndex::UM_WORD_INDEX_MEMORY_FACTOR;
                    if (trie_type == TC2DHashMapTrie_STR) {
                        return new C2DHashMapTrie<N>(new BasicWordIndex(memory_factor));
                    } else {
                        if (trie_type == TW2CHybridMemoryTrie_STR) {
                            return new typename TW2CHybridMemoryTrie<N>::type(new BasicWordIndex(memory_factor));
                        } else {
                            if (trie_type == TC2WOrderedArrayTrie_STR) {
                                return new C2WOrderedArrayTrie<N>(new BasicWordIndex(memory_factor));
                            } else {
                                if (trie_type == TW2COrderedArrayTrie_STR) {
                                    return new W2COrderedArrayTrie<N>(new BasicWordIndex(memory_factor));
                                } else {
                                    if (trie_type == C2DMapArrayTrie_STR) {
                                        return new C2DMapArrayTrie<N>(new BasicWordIndex(memory_factor));
                                    } else {
                                        if (trie_type == G2DHashMapTrie_STR) {
                                            return new G2DHashMapTrie<N>(new CountingWordIndex(memory_factor));
                                        } else {
                                            stringstream msg;
                                            msg << "Unrecognized trie type: " + trie_type;
                                            throw Exception(msg.str());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                /**
                 * Allows to get a string with all available (known to the factory) trie types
                 * @return the string with trie types, to be use from command line
                 */
                static inline string getTrieTypesStr() {
                    stringstream text;
                    text << "{" << TC2DHashMapTrie_STR << ", "
                            << TW2CHybridMemoryTrie_STR << ", "
                            << TC2WOrderedArrayTrie_STR << ", "
                            << TW2COrderedArrayTrie_STR << ", "
                            << C2DMapArrayTrie_STR << ", "
                            << G2DHashMapTrie_STR << "}";
                    return text.str();
                }
            };

            //Initialize constants
            const string TrieTypeFactory::TC2DHashMapTrie_STR = string("c2dm");
            const string TrieTypeFactory::TW2CHybridMemoryTrie_STR = string("w2ch");
            const string TrieTypeFactory::TC2WOrderedArrayTrie_STR = string("c2wa");
            const string TrieTypeFactory::TW2COrderedArrayTrie_STR = string("w2ca");
            const string TrieTypeFactory::C2DMapArrayTrie_STR = string("c2dh");
            const string TrieTypeFactory::G2DHashMapTrie_STR = string("g2dm");
        }
    }
}

#endif	/* TRIETYPEFACTORY_HPP */

