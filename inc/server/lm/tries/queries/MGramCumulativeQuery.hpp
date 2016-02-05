/* 
 * File:   MGramCumulativeQuery.hpp
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
 * Created on November 5, 2015, 3:59 PM
 */

#ifndef MGRAMCUMULATIVEQUERY_HPP
#define MGRAMCUMULATIVEQUERY_HPP

#include <string>       //std::string

#include "server/lm/trie_constants.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/mgrams/QueryMGram.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "server/lm/tries/queries/MGramQuery.hpp"

#include "server/lm/dictionaries/AWordIndex.hpp"
#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::translation::server::lm;
using namespace uva::smt::translation::server::lm::dictionary;
using namespace uva::smt::translation::server::lm::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {

                    /**
                     * Stores the query and its internal for the sake of re-usability and
                     * independency from the Tries and executor. Allows to compute 
                     *      \Sum_{1}^{5}log_10(Prob(w_{i}|w_{1}...w_{i-1}))
                     * where log_10(Prob(w_{i}|w_{1}...w_{i-1})) is > ZERO_LOG_PROB_WEIGHT
                     * 
                     * Note that, here 5 is taken just as an example.
                     */
                    template<typename TrieType>
                    class T_M_Gram_Cumulative_Query : public T_M_Gram_Query<TrieType> {
                    public:
                        //The word index type
                        typedef typename TrieType::WordIndexType WordIndexType;
                        //Define the base class type
                        typedef T_M_Gram_Query<TrieType> BASE;

                        //Define the maximum level constant
                        static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;

                        /**
                         * The basic constructor for the structure
                         * @param trie the reference to the trie object
                         */
                        T_M_Gram_Cumulative_Query(TrieType & trie) : T_M_Gram_Query<TrieType>(trie) {
                        }

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        inline void log_results() const {
                            BASE::log_cumulative_results();
                        }

                        /**
                         * Allows to execute m-gram the query
                         * @param text the piece containing the m-gram query
                         */
                        inline void execute(TextPieceReader &text) {
                            BASE::template execute<true>(text);
                        }
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Cumulative_Query<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Cumulative_Query<H2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

                    INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL_MAX);
                }
            }
        }
    }
}

#endif /* MGRAMCUMULATIVEQUERY_HPP */

