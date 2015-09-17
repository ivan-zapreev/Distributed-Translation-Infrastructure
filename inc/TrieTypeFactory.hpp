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
                static const string TC2DMapTrie_STR;
                static const string TW2CHybridTrie_STR;
                static const string TC2WArrayTrie_STR;
                static const string TW2CArrayTrie_STR;
                static const string C2DHybridTrie_STR;
                static const string G2DMapTrie_STR;

            public:

                /**
                 * Allows to instantiate an appropriate word index 
                 * @param word_index_type the word index type
                 * @return pointer to a new word index instance
                 */
                static inline AWordIndex * getWordIndex(const WordIndexTypesEnum word_index_type) {
                    const size_t memory_factor = __HashMapWordIndex::MEMORY_FACTOR;
                    switch (word_index_type) {
                        case WordIndexTypesEnum::BASIC_WORD_INDEX:
                            LOG_USAGE << "Using BASIC_WORD_INDEX to instantiate the trie." << END_LOG;
                            return new BasicWordIndex(memory_factor);
                        case WordIndexTypesEnum::COUNTING_WORD_INDEX:
                            LOG_USAGE << "Using COUNTING_WORD_INDEX to instantiate the trie." << END_LOG;
                            return new CountingWordIndex(memory_factor);
                        default:
                            stringstream msg;
                            msg << "Unrecognized word index type: " << word_index_type;
                            throw Exception(msg.str());
                    }
                }

                /**
                 * Instantiate a trie for the given string name
                 * @param trie_type the trie type name
                 * @return the instance of trie
                 * @throws Exception if the trie type name is not recognized
                 */
                template<TModelLevel N>
                static inline ATrie<N>* getTrie(const string trie_type) {
                    if (trie_type == TC2DMapTrie_STR) {
                        return new C2DMapTrie<N>(getWordIndex(__C2DMapTrie::WORD_INDEX_TYPE));
                    } else {
                        if (trie_type == TW2CHybridTrie_STR) {
                            return new typename TW2CHybridTrie<N>::type(getWordIndex(__W2CHybridTrie::WORD_INDEX_TYPE));
                        } else {
                            if (trie_type == TC2WArrayTrie_STR) {
                                return new C2WArrayTrie<N>(getWordIndex(__C2WArrayTrie::WORD_INDEX_TYPE));
                            } else {
                                if (trie_type == TW2CArrayTrie_STR) {
                                    return new W2CArrayTrie<N>(getWordIndex(__W2CArrayTrie::WORD_INDEX_TYPE));
                                } else {
                                    if (trie_type == C2DHybridTrie_STR) {
                                        return new C2DHybridTrie<N>(getWordIndex(__C2DHybridTrie::WORD_INDEX_TYPE));
                                    } else {
                                        if (trie_type == G2DMapTrie_STR) {
                                            return new G2DMapTrie<N>(getWordIndex(__G2DMapTrie::WORD_INDEX_TYPE));
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
                    text << "{" << TC2DMapTrie_STR << ", "
                            << TW2CHybridTrie_STR << ", "
                            << TC2WArrayTrie_STR << ", "
                            << TW2CArrayTrie_STR << ", "
                            << C2DHybridTrie_STR << ", "
                            << G2DMapTrie_STR << "}";
                    return text.str();
                }
            };

            //Initialize constants
            const string TrieTypeFactory::TC2DMapTrie_STR = string("c2dm");
            const string TrieTypeFactory::TW2CHybridTrie_STR = string("w2ch");
            const string TrieTypeFactory::TC2WArrayTrie_STR = string("c2wa");
            const string TrieTypeFactory::TW2CArrayTrie_STR = string("w2ca");
            const string TrieTypeFactory::C2DHybridTrie_STR = string("c2dh");
            const string TrieTypeFactory::G2DMapTrie_STR = string("g2dm");
        }
    }
}

#endif	/* TRIETYPEFACTORY_HPP */

