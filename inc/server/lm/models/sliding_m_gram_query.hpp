/* 
 * File:   sliding_m_gram_query.hpp
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
 * Created on February 23, 2016, 3:28 PM
 */

#ifndef SLIDING_M_GRAM_QUERY_HPP
#define SLIDING_M_GRAM_QUERY_HPP

#include <string>       //std::string

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_configs.hpp"

#include "server/lm/mgrams/query_m_gram.hpp"
#include "server/lm/models/generic_trie_base.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"

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
                     * independency from the Tries and executor.
                     */
                    template<typename TrieType>
                    class sliding_m_gram_query {
                    public:
                        typedef typename TrieType::WordIndexType WordIndexType;

                        /**
                         * The basic constructor for the structure
                         * @param trie the reference to the trie object
                         */
                        sliding_m_gram_query(const TrieType & trie)
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
                         * @param word_ids an array of word ids of the phrase, the length must be equal to LM_QUERY_LENGTH_MAX
                         */
                        template<bool is_cumulative, bool is_log_results = false >
                        inline prob_weight execute(const uint64_t * word_ids) {
                            
                            //ToDo: Implement the query with the sliding window
                            
                            return ZERO_LOG_PROB_WEIGHT;
                        }

                    protected:
                        //Stores the reference to the constant trie.
                        const TrieType & m_trie;

                        //Define the query data structure that: stores the query m-gram,
                        //stores pointers to the retrieved payloads, stores the computed
                        //conditional probabilities per sub-m-gram and others
                        typename TrieType::query_exec_data m_query;
                    };

                    //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_SLIDING_M_GRAM_QUERY_WORD_IDX(WORD_INDEX_TYPE); \
            template class sliding_m_gram_query<C2DHybridTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<C2DMapTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<C2WArrayTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<W2CArrayTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<W2CHybridTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<G2DMapTrie<WORD_INDEX_TYPE>>; \
            template class sliding_m_gram_query<H2DMapTrie<WORD_INDEX_TYPE>>;

                    INSTANTIATE_SLIDING_M_GRAM_QUERY_WORD_IDX(basic_word_index);
                    INSTANTIATE_SLIDING_M_GRAM_QUERY_WORD_IDX(counting_word_index);
                    INSTANTIATE_SLIDING_M_GRAM_QUERY_WORD_IDX(TOptBasicWordIndex);
                    INSTANTIATE_SLIDING_M_GRAM_QUERY_WORD_IDX(TOptCountWordIndex);
                }
            }
        }
    }
}

#endif /* SLIDING_M_GRAM_QUERY_HPP */

