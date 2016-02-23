/* 
 * File:   query_exec_data.hpp
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
 * Created on February 23, 2016, 1:28 PM
 */

#ifndef QUERY_EXEC_DATA_HPP
#define QUERY_EXEC_DATA_HPP


using namespace std;
using namespace uva::utils::logging;

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_configs.hpp"

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/mgrams/query_m_gram.hpp"

using namespace uva::smt::bpbd::server::lm::m_grams;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This structure stores the basic data required for a query execution.
                     * @param m_query the m-gram query itself 
                     * @param m_payloads the  two dimensional array of the payloads 
                     * @param m_last_ctx_ids stores the last context id computed for the given row of the sub-m-gram matrix
                     * @param m_probs the array f probabilities 
                     * @param m_begin_word_idx the currently considered begin word index
                     * @param m_end_word_idx the currently considered end word index
                     */
                    template<typename word_idx_type>
                    struct query_exec_data_templ{
                        query_m_gram<word_idx_type> m_gram;
                        
                        const void * m_payloads[LM_M_GRAM_LEVEL_MAX][LM_M_GRAM_LEVEL_MAX];
                        
                        TLongId m_last_ctx_ids[LM_M_GRAM_LEVEL_MAX];
                        
                        TLogProbBackOff m_probs[LM_M_GRAM_LEVEL_MAX];
                        
                        TModelLevel m_begin_word_idx;
                        TModelLevel m_end_word_idx;

                        /**
                         * The basic constructor that gets a reference to the word index
                         * @param word_index the reference to the word index
                         */
                        explicit query_exec_data_templ(word_idx_type & word_index) : m_gram(word_index) {
                        }
                    };
                }
            }
        }
    }
}

#endif /* QUERY_EXEC_DATA_HPP */

