/* 
 * File:   configurator.hpp
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
 * Created on February 4, 2016, 1:32 PM
 */

#ifndef CONFIGURATOR_HPP
#define CONFIGURATOR_HPP

#include "server/lm/lm_parameters.hpp"

#include "server/lm/tries/C2DMapTrie.hpp"
#include "server/lm/tries/W2CHybridTrie.hpp"
#include "server/lm/tries/C2WArrayTrie.hpp"
#include "server/lm/tries/W2CArrayTrie.hpp"
#include "server/lm/tries/C2DHybridTrie.hpp"
#include "server/lm/tries/G2DMapTrie.hpp"
#include "server/lm/tries/H2DMapTrie.hpp"

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {

                    //Initialize constants
                    static const string TC2DMapTrie_STR = string("c2dm");
                    static const string TW2CHybridTrie_STR = string("w2ch");
                    static const string TC2WArrayTrie_STR = string("c2wa");
                    static const string TW2CArrayTrie_STR = string("w2ca");
                    static const string C2DHybridTrie_STR = string("c2dh");
                    static const string G2DMapTrie_STR = string("g2dm");
                    static const string H2DMapTrie_STR = string("h2dm");

                    namespace __configurator {

                        /**
                         * Based on the trie name stored in the parameters allows
                         * to determine the trie type and the appropriate word
                         * index for the trie.
                         * @param params the parameters storing structure in/out
                         */
                        static void get_trie_and_word_index_types(lm_parameters& params) {
                            if (params.m_trie_type_name == TC2DMapTrie_STR) {
                                params.m_word_index_type = __C2DMapTrie::WORD_INDEX_TYPE;
                                params.m_trie_type = TrieTypesEnum::C2DM_TRIE;
                            } else {
                                if (params.m_trie_type_name == TW2CHybridTrie_STR) {
                                    params.m_word_index_type = __W2CHybridTrie::WORD_INDEX_TYPE;
                                    params.m_trie_type = TrieTypesEnum::W2CH_TRIE;
                                } else {
                                    if (params.m_trie_type_name == TC2WArrayTrie_STR) {
                                        params.m_word_index_type = __C2WArrayTrie::WORD_INDEX_TYPE;
                                        params.m_trie_type = TrieTypesEnum::C2WA_TRIE;
                                    } else {
                                        if (params.m_trie_type_name == TW2CArrayTrie_STR) {
                                            params.m_word_index_type = __W2CArrayTrie::WORD_INDEX_TYPE;
                                            params.m_trie_type = TrieTypesEnum::W2CA_TRIE;
                                        } else {
                                            if (params.m_trie_type_name == C2DHybridTrie_STR) {
                                                params.m_word_index_type = __C2DHybridTrie::WORD_INDEX_TYPE;
                                                params.m_trie_type = TrieTypesEnum::C2DH_TRIE;
                                            } else {
                                                if (params.m_trie_type_name == G2DMapTrie_STR) {
                                                    params.m_word_index_type = __G2DMapTrie::WORD_INDEX_TYPE;
                                                    params.m_trie_type = TrieTypesEnum::G2DM_TRIE;
                                                } else {
                                                    if (params.m_trie_type_name == H2DMapTrie_STR) {
                                                        params.m_word_index_type = __H2DMapTrie::WORD_INDEX_TYPE;
                                                        params.m_trie_type = TrieTypesEnum::H2DM_TRIE;
                                                    } else {
                                                        THROW_EXCEPTION(string("Unrecognized trie type: ") + params.m_trie_type_name);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    }
                }
            }
        }
    }
}

#endif /* CONFIGURATOR_HPP */

