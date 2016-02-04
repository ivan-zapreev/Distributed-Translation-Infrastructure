/* 
 * File:   MGramSingleQuery.hpp
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

#ifndef MGRAMSINGLEQUERY_HPP
#define MGRAMSINGLEQUERY_HPP

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
                     *      log_10(Prob(w_{5}|w_{1}w_{2}w_{3}w_{4}))
                     * Note that, here 5 is taken just as an example.
                     */
                    template<typename TrieType>
                    class T_M_Gram_Single_Query : public T_M_Gram_Query<TrieType> {
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
                        T_M_Gram_Single_Query(TrieType & trie) : T_M_Gram_Query<TrieType>(trie) {
                        }

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        inline void log_results() const {
                            //Print the query results
                            const string gram_str = BASE::m_query.m_gram.get_mgram_prob_str(BASE::m_query.m_gram.get_m_gram_level());

                            LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                    << " ) ) = " << SSTR(BASE::m_query.m_probs[BASE::m_query.m_gram.get_end_word_idx()]) << END_LOG;
                            LOG_INFO << "  Prob( " << gram_str << " ) = "
                                    << SSTR(pow(LOG_PROB_WEIGHT_BASE, BASE::m_query.m_probs[BASE::m_query.m_gram.get_end_word_idx()])) << END_LOG;

                            LOG_RESULT << "-------------------------------------------" << END_LOG;
                        }

                        /**
                         * Allows to execute m-gram the query
                         * @param text the piece containing the m-gram query
                         */
                        void execute(TextPieceReader &text) {
                            LOG_DEBUG << "Starting to execute:" << (string) BASE::m_query.m_gram << END_LOG;

                            //Set the text piece into the m-gram
                            BASE::m_query.m_gram.set_m_gram_from_text(text);

                            //Clean the relevant probability entry
                            BASE::m_query.m_probs[ BASE::m_query.m_gram.get_end_word_idx() ] = ZERO_PROB_WEIGHT;
                            //Clean the payload pointer entries
                            memset(BASE::m_query.m_payloads, 0, sizeof (void*) * MAX_LEVEL * MAX_LEVEL);

                            //If this trie needs getting context ids then clean the data as well
                            if (BASE::m_trie.is_need_getting_ctx_ids()) {
                                //Clean the payload pointer entries
                                memset(BASE::m_query.m_last_ctx_ids, WordIndexType::UNDEFINED_WORD_ID, sizeof (TLongId) * MAX_LEVEL);
                            }

                            //Execute the query
                            BASE::m_trie.template execute<false>(BASE::m_query);

                            LOG_DEBUG << "Finished executing:" << (string) BASE::m_query.m_gram << END_LOG;
                        }
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Single_Query<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Single_Query<H2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

                    INSTANTIATE_TYPEDEF_M_GRAM_SINGLE_QUERY_LEVEL(M_GRAM_LEVEL_MAX);
                }
            }
        }
    }
}
#endif /* MGRAMSINGLEQUERY_HPP */

