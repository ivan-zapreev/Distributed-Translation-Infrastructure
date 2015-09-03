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
#include "ALayeredTrie.hpp"
#include "HashMapWordIndex.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "HashMapWordIndex.hpp"
#include "C2DMapArrayTrie.hpp"

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
                static const string TCtxMultiHashMapTrie_STR;
                static const string TMapW2CHybridTrie_STR;
                static const string TC2WOrderedArrayTrie_STR;
                static const string TW2COrderedArrayTrie_STR;
                static const string C2DMapArrayTrie_STR;

            public:

                /**
                 * Instantiate a trie for the given string name
                 * @param trie_type the trie type name
                 * @param dictionary the dictionary to be used
                 * @return the instance of trie
                 * @throws Exception if the trie type name is not recognized
                 */
                template<TModelLevel N>
                static inline ALayeredTrie<N>* getTrie(const string trie_type, HashMapWordIndex &dictionary) {
                    if (trie_type == TCtxMultiHashMapTrie_STR) {
                        return new TCtxMultiHashMapTrie_N5(&dictionary);
                    } else {
                        if (trie_type == TMapW2CHybridTrie_STR) {
                            return new TMapW2CHybridTrie_N5(&dictionary);
                        } else {
                            if (trie_type == TC2WOrderedArrayTrie_STR) {
                                return new TC2WOrderedArrayTrie_N5(&dictionary);
                            } else {
                                if (trie_type == TW2COrderedArrayTrie_STR) {
                                    return new TW2COrderedArrayTrie_N5(&dictionary);
                                } else {
                                    if (trie_type == C2DMapArrayTrie_STR) {
                                        return new TC2DMapArrayTrie_N5(&dictionary);
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

                /**
                 * Allows to get a string with all available (known to the factory) trie types
                 * @return the string with trie types, to be use from command line
                 */
                static inline string getTrieTypesStr() {
                    stringstream text;
                    text << "{" << TCtxMultiHashMapTrie_STR << ", "
                            << TMapW2CHybridTrie_STR << ", "
                            << TC2WOrderedArrayTrie_STR << ", "
                            << TW2COrderedArrayTrie_STR << ", "
                            << C2DMapArrayTrie_STR << "}";
                    return text.str();
                }
            };

            //Initialize constants
            const string TrieTypeFactory::TCtxMultiHashMapTrie_STR = string("c2dm");
            const string TrieTypeFactory::TMapW2CHybridTrie_STR = string("w2ch");
            const string TrieTypeFactory::TC2WOrderedArrayTrie_STR = string("c2wa");
            const string TrieTypeFactory::TW2COrderedArrayTrie_STR = string("w2ca");
            const string TrieTypeFactory::C2DMapArrayTrie_STR = string("c2dh"); 
        }
    }
}

#endif	/* TRIETYPEFACTORY_HPP */

