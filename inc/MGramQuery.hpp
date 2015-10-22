/* 
 * File:   MGramQuery.hpp
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
 * Created on September 18, 2015, 7:41 PM
 */

#ifndef MGRAMQUERY_HPP
#define	MGRAMQUERY_HPP

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MGrams.hpp"
#include "TextPieceReader.hpp"

#include "AWordIndex.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieDriver.hpp"
#include "LayeredTrieDriver.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "G2DHashMapTrie.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::smt::exceptions;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * Stores the query and its internal for the sake of re-usability and
             * independency from the Tries and executor.
             */
            template<typename TrieType>
            struct T_M_Gram_Query {
                //Stores the reference to the constant trie.
                const TrieType & m_trie;
                
                //Stores the query m-gram
                T_M_Gram<typename TrieType::WordIndexType> m_gram;

                //Stores the query result
                TQueryResult m_result = {};

                //Stores the current end word index during the query execution
                TModelLevel m_curr_end_word_idx = 0;

                /**
                 * The basic constructor for the structure
                 * @param trie the reference to the trie object
                 */
                T_M_Gram_Query(TrieType & trie) : m_trie(trie), m_gram(trie.get_word_index()) {
                }

                /**
                 * Allows t set a new query into this state object
                 * @param m_gram the query M-gram
                 * @return the m-gram's level
                 */
                inline TModelLevel prepare_query() {
                    //Set the result probability to zero
                    m_result.m_prob = ZERO_PROB_WEIGHT;

                    //Prepare the m-gram for querying
                    m_gram.prepare_for_querying();

                    //return the m-gram level
                    return m_gram.m_used_level;
                }

                /**
                 * Gets the word hash for the end word of the back-off M-Gram
                 * @return the word hash for the end word of the back-off M-Gram
                 */
                inline TShortId get_back_off_end_word_id() const {
                    return m_gram.get_back_off_end_word_id();
                }

                /**
                 * Gets the word hash for the last word in the M-gram
                 * @return the word hash for the last word in the M-gram
                 */
                inline TShortId get_end_word_id() const {
                    return m_gram.get_end_word_id();
                }

                /**
                 * Allows to check if the given back-off sub-m-gram contains 
                 * an unknown word for the given current level.
                 */
                template<bool is_back_off, TModelLevel curr_level>
                inline bool has_no_unk_words() const {
                    return m_gram.template has_no_unk_words<is_back_off, curr_level>();
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template struct T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template struct T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template struct T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template struct T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template struct T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template struct T_M_Gram_Query<GenericTrieDriver<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_2);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_3);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_4);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_5);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_6);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_7);
        }
    }
}

#endif	/* QUERYSTATE_HPP */

