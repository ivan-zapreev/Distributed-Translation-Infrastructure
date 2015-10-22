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

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "LayeredTrieDriver.hpp"
#include "G2DHashMapTrie.hpp"
#include "GenericTrieDriver.hpp"

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
            class T_M_Gram_Query {
            public:

                //Stores the query m-gram
                T_M_Gram<typename TrieType::WordIndexType> m_gram;

                //Stores the query result
                TQueryResult m_result = {};

                /**
                 * The basic constructor for the structure
                 * @param trie the reference to the trie object
                 */
                T_M_Gram_Query(TrieType & trie) : m_gram(trie.get_word_index()),  m_trie(trie) {
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param gram the given M-Gram query and its state
                 * @param result the structure to store the resulting probability
                 */
                void execute() {
                    LOG_DEBUG << "Starting to execute:" << (string) m_gram << END_LOG;

                    //Set the result probability to zero
                    m_result.m_prob = ZERO_PROB_WEIGHT;

                    //Prepare the m-gram for querying
                    m_gram.prepare_for_querying();

                    //Get the used level and go on with execution
                    TModelLevel curr_level = m_gram.m_actual_level;

                    //Compute the probability in the loop fashion, should be faster that recursion.
                    while (m_result.m_prob == ZERO_PROB_WEIGHT) {
                        //Do a sanity check if needed
                        if (DO_SANITY_CHECKS && (curr_level < M_GRAM_LEVEL_1)) {
                            stringstream msg;
                            msg << "An impossible value of curr_level: " << SSTR(curr_level)
                                    << ", it must be >= " << SSTR(M_GRAM_LEVEL_1);
                            throw Exception(msg.str());
                        }

                        //Try to compute the next probability with decreased level
                        m_trie.get_prob_weight(curr_level, m_gram, m_result);

                        //Decrease the level
                        curr_level--;
                    }

                    LOG_DEBUG << "The current level value is: " << curr_level
                            << ", the current probability value is: " << m_result.m_prob << END_LOG;

                    //If the probability is log-zero or snaller then there is no
                    //need for a back-off as then we will only get smaller values.
                    if (m_result.m_prob > ZERO_LOG_PROB_WEIGHT) {
                        //If the curr_level is smaller than the original level then
                        //it means that we needed to back-off, add back-off weights
                        for (++curr_level; curr_level != m_gram.m_actual_level; ++curr_level) {
                            //Get the back_off 
                            m_trie.add_back_off_weight(curr_level, m_gram, m_result);
                        }
                    }

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE
                            << " probability is: " << m_result.m_prob << END_LOG;
                }

            private:
                //Stores the reference to the constant trie.
                const TrieType & m_trie;
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Query<GenericTrieDriver<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL(M_GRAM_LEVEL_MAX);
        }
    }
}

#endif	/* QUERYSTATE_HPP */

