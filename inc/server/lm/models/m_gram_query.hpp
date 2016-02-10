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
#define MGRAMQUERY_HPP

#include <string>       //std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/mgrams/QueryMGram.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "server/lm/models/generic_trie_base.hpp"

#include "server/lm/dictionaries/AWordIndex.hpp"
#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * Stores the query and its internal for the sake of re-usability and
                     * independency from the Tries and executor. Allows to compute 
                     *      log_10(Prob(w_{5}|w_{1}w_{2}w_{3}w_{4}))
                     * or
                     *      \Sum_{1}^{5}log_10(Prob(w_{i}|w_{1}...w_{i-1}))
                     * where log_10(Prob(w_{i}|w_{1}...w_{i-1})) is > ZERO_LOG_PROB_WEIGHT
                     * depending on the value of the IS_CUMULATIVE template parameter.
                     * 
                     * Note that, here 5 is taken just as an example.
                     * 
                     * @param IS_CUMULATIVE if false then for the given M-gram only the
                     * conditional log probability is computed, if true then we compute
                     * the conditional probability of all sub M-grams and also the total
                     * sum, not taking into account the zero log probabilities.
                     */
                    template<typename TrieType>
                    class T_M_Gram_Query {
                    public:
                        typedef typename TrieType::WordIndexType WordIndexType;

                        //Define the maximum level constant
                        static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;

                        /**
                         * The basic constructor for the structure
                         * @param trie the reference to the trie object
                         */
                        T_M_Gram_Query(const TrieType & trie)
                        : m_trie(trie), m_query(trie.get_word_index()) {
                        }

                        /**
                         * Allows to execute m-gram the query
                         * @param text the piece containing the m-gram query
                         */
                        template<bool is_cumulative, bool is_log_results = false >
                        inline void execute(TextPieceReader &text) {
                            LOG_DEBUG << "Starting to execute:" << (string) m_query.m_gram << END_LOG;

                            //Set the text piece into the m-gram
                            m_query.m_gram.set_m_gram_from_text(text);

                            //Clean the relevant probability entry
                            if (is_cumulative) {
                                memset(m_query.m_probs, 0, sizeof (TLogProbBackOff) * MAX_LEVEL);
                            } else {
                                m_query.m_probs[ m_query.m_gram.get_end_word_idx() ] = ZERO_PROB_WEIGHT;
                            }
                            //Clean the payload pointer entries
                            memset(m_query.m_payloads, 0, sizeof (void*) * MAX_LEVEL * MAX_LEVEL);

                            //If this trie needs getting context ids then clean the data as well
                            if (m_trie.is_need_getting_ctx_ids()) {
                                //Clean the payload pointer entries
                                memset(m_query.m_last_ctx_ids, WordIndexType::UNDEFINED_WORD_ID, sizeof (TLongId) * MAX_LEVEL);
                            }

                            //Execute the query
                            m_trie.template execute<is_cumulative>(m_query);

                            LOG_DEBUG << "Finished executing:" << (string) m_query.m_gram << END_LOG;

                            //Log the results if needed
                            if (is_log_results) {
                                if (is_cumulative) {
                                    log_cumulative_results();
                                } else {
                                    log_single_results();
                                }
                            }
                        }

                    protected:
                        //Stores the reference to the constant trie.
                        const TrieType & m_trie;

                        //Define the query data structure that: stores the query m-gram,
                        //stores pointers to the retrieved payloads, stores the computed
                        //conditional probabilities per sub-m-gram and others
                        typename TrieType::T_Query_Exec_Data m_query;

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        inline void log_single_results() const {
                            //Print the query results
                            const string gram_str = m_query.m_gram.get_mgram_prob_str(m_query.m_gram.get_m_gram_level());

                            LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                    << " ) ) = " << SSTR(m_query.m_probs[m_query.m_gram.get_end_word_idx()]) << END_LOG;
                            LOG_INFO << "  Prob( " << gram_str << " ) = "
                                    << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_query.m_probs[m_query.m_gram.get_end_word_idx()])) << END_LOG;

                            LOG_RESULT << "-------------------------------------------" << END_LOG;
                        }

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        inline void log_cumulative_results() const {
                            //Initialize the current index, with the proper start value
                            TModelLevel curr_idx = m_query.m_gram.get_begin_word_idx();
                            TLogProbBackOff cumulative_prob = ZERO_PROB_WEIGHT;

                            //Print the intermediate results
                            for (; curr_idx <= m_query.m_gram.get_end_word_idx(); ++curr_idx) {
                                const string gram_str = m_query.m_gram.get_mgram_prob_str(curr_idx + 1);
                                LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                        << " ) ) = " << SSTR(m_query.m_probs[curr_idx]) << END_LOG;
                                LOG_INFO << "  Prob( " << gram_str << " ) = "
                                        << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_query.m_probs[curr_idx])) << END_LOG;
                                if (m_query.m_probs[curr_idx] > ZERO_LOG_PROB_WEIGHT) {
                                    cumulative_prob += m_query.m_probs[curr_idx];
                                }
                            }
                            LOG_RESULT << "---" << END_LOG;

                            //Print the total cumulative probability if needed
                            const string gram_str = m_query.m_gram.get_mgram_prob_str();
                            LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                    << " ) ) = " << SSTR(cumulative_prob) << END_LOG;
                            LOG_INFO << "  Prob( " << gram_str << " ) = "
                                    << SSTR(pow(LOG_PROB_WEIGHT_BASE, cumulative_prob)) << END_LOG;

                            LOG_RESULT << "-------------------------------------------" << END_LOG;
                        }
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_M_GRAM_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Query<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>; \
            template class T_M_Gram_Query<H2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>;

#define INSTANTIATE_M_GRAM_QUERY_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_M_GRAM_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_M_GRAM_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_M_GRAM_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_M_GRAM_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

                    INSTANTIATE_M_GRAM_QUERY_LEVEL(M_GRAM_LEVEL_MAX);
                }
            }
        }
    }
}

#endif /* QUERYSTATE_HPP */

