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

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_configs.hpp"
#include "common/utils/file/text_piece_reader.hpp"

#include "server/lm/mgrams/query_m_gram.hpp"
#include "server/lm/models/generic_trie_base.hpp"

#include "server/lm/dictionaries/BasicWordIndex.hpp"
#include "server/lm/dictionaries/CountingWordIndex.hpp"
#include "server/lm/dictionaries/OptimizingWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"

#include "server/lm/models/c2d_hybrid_trie.hpp"
#include "server/lm/models/c2d_map_trie.hpp"
#include "server/lm/models/c2w_array_trie.hpp"
#include "server/lm/models/g2d_map_trie.hpp"
#include "server/lm/models/h2d_map_trie.hpp"
#include "server/lm/models/w2c_array_trie.hpp"
#include "server/lm/models/w2c_hybrid_trie.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::math::bits;
using namespace uva::utils::file;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::m_grams;

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
                    class m_gram_query {
                    public:
                        typedef typename TrieType::WordIndexType WordIndexType;

                        /**
                         * The basic constructor for the structure
                         * @param trie the reference to the trie object
                         */
                        m_gram_query(const TrieType & trie)
                        : m_trie(trie), m_query(trie.get_word_index()) {
                        }

                        /**
                         * Allows to retrieve the unknown target word log probability penalty 
                         * @return the target source word log probability penalty
                         */
                        inline float get_unk_word_prob() {
                            return m_trie.get_unk_word_prob();
                        }

                        /**
                         * Allows to execute m-gram the query
                         * @param text the piece containing the m-gram query
                         */
                        template<bool is_cumulative, bool is_log_results = false >
                        inline TLogProbBackOff execute(TextPieceReader &text) {
                            LOG_DEBUG << "Starting to execute: ___" << text << "___" << END_LOG;

                            //Set the text piece into the m-gram
                            m_query.m_gram.set_m_gram_from_text(text);

                            LOG_DEBUG << "The parsed m-gram is:" << (string) m_query.m_gram << END_LOG;

                            //Clean the relevant probability entry
                            if (is_cumulative) {
                                memset(m_query.m_probs, 0, sizeof (TLogProbBackOff) * QUERY_LENGTH_MAX);
                            } else {
                                m_query.m_probs[ m_query.m_gram.get_end_word_idx() ] = ZERO_PROB_WEIGHT;
                            }
                            //Clean the payload pointer entries
                            memset(m_query.m_payloads, 0, sizeof (void*) * QUERY_LENGTH_MAX * QUERY_LENGTH_MAX);

                            //If this trie needs getting context ids then clean the data as well
                            if (m_trie.is_need_getting_ctx_ids()) {
                                //Clean the payload pointer entries
                                memset(m_query.m_last_ctx_ids, WordIndexType::UNDEFINED_WORD_ID, sizeof (TLongId) * QUERY_LENGTH_MAX);
                            }

                            //Execute the query
                            m_trie.template execute<is_cumulative>(m_query);

                            LOG_DEBUG << "Finished executing:" << (string) m_query.m_gram << END_LOG;

                            //Return the results
                            if (is_cumulative) {
                                return get_cumulative_result<is_log_results>();
                            } else {
                                return get_single_result<is_log_results>();
                            }
                        }

                    protected:
                        //Stores the reference to the constant trie.
                        const TrieType & m_trie;

                        //Define the query data structure that: stores the query m-gram,
                        //stores pointers to the retrieved payloads, stores the computed
                        //conditional probabilities per sub-m-gram and others
                        typename TrieType::query_exec_data m_query;

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        template<bool is_log_results>
                        inline TLogProbBackOff get_single_result() const {
                            //Print the query results
                            const string gram_str = m_query.m_gram.get_mgram_prob_str(m_query.m_gram.get_m_gram_level());

                            TLogProbBackOff single_prob = m_query.m_probs[m_query.m_gram.get_end_word_idx()];

                            if (is_log_results) {
                                LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                        << " ) ) = " << SSTR(single_prob) << END_LOG;
                                LOG_INFO << "  Prob( " << gram_str << " ) = "
                                        << SSTR(pow(LOG_PROB_WEIGHT_BASE, single_prob)) << END_LOG;

                                LOG_RESULT << "-------------------------------------------" << END_LOG;
                            }

                            //Return the result
                            return single_prob;
                        }

                        /**
                         * Allows to log the query results after its execution.
                         * Different logging is done based on enabled logging level
                         * and the class template parameters.
                         */
                        template<bool is_log_results>
                        inline TLogProbBackOff get_cumulative_result() const {
                            //Initialize the current index, with the proper start value
                            TModelLevel curr_idx = m_query.m_gram.get_begin_word_idx();
                            TLogProbBackOff cumulative_prob = ZERO_PROB_WEIGHT;

                            //Print the intermediate results
                            for (; curr_idx <= m_query.m_gram.get_end_word_idx(); ++curr_idx) {
                                const string gram_str = m_query.m_gram.get_mgram_prob_str(curr_idx + 1);

                                if (is_log_results) {
                                    LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                            << " ) ) = " << SSTR(m_query.m_probs[curr_idx]) << END_LOG;
                                    LOG_INFO << "  Prob( " << gram_str << " ) = "
                                            << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_query.m_probs[curr_idx])) << END_LOG;
                                }

                                if (m_query.m_probs[curr_idx] > ZERO_LOG_PROB_WEIGHT) {
                                    cumulative_prob += m_query.m_probs[curr_idx];
                                }
                            }

                            if (is_log_results) {
                                LOG_RESULT << "---" << END_LOG;
                                //Print the total cumulative probability if needed
                                const string gram_str = m_query.m_gram.get_mgram_prob_str();
                                LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                        << " ) ) = " << SSTR(cumulative_prob) << END_LOG;
                                LOG_INFO << "  Prob( " << gram_str << " ) = "
                                        << SSTR(pow(LOG_PROB_WEIGHT_BASE, cumulative_prob)) << END_LOG;

                                LOG_RESULT << "-------------------------------------------" << END_LOG;
                            }

                            //Return the result
                            return cumulative_prob;
                        }
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_M_GRAM_QUERY_WORD_IDX(WORD_INDEX_TYPE); \
            template class m_gram_query<C2DHybridTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<C2DMapTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<C2WArrayTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<W2CArrayTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<W2CHybridTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<G2DMapTrie<WORD_INDEX_TYPE>>; \
            template class m_gram_query<H2DMapTrie<WORD_INDEX_TYPE>>;

                    INSTANTIATE_M_GRAM_QUERY_WORD_IDX(BasicWordIndex);
                    INSTANTIATE_M_GRAM_QUERY_WORD_IDX(CountingWordIndex);
                    INSTANTIATE_M_GRAM_QUERY_WORD_IDX(TOptBasicWordIndex);
                    INSTANTIATE_M_GRAM_QUERY_WORD_IDX(TOptCountWordIndex);
                }
            }
        }
    }
}

#endif /* QUERYSTATE_HPP */

