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

#include "server/lm/TrieConstants.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/mgrams/QueryMGram.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "server/lm/tries/GenericTrieBase.hpp"

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
                        T_M_Gram_Query(TrieType & trie)
                        : m_trie(trie), m_query(trie.get_word_index()) {
                        }

                    protected:
                        //Stores the reference to the constant trie.
                        const TrieType & m_trie;

                        //Define the query data structure that: stores the query m-gram,
                        //stores pointers to the retrieved payloads, stores the computed
                        //conditional probabilities per sub-m-gram and others
                        typename TrieType::T_Query_Exec_Data m_query;
                    };
                }
            }
        }
    }
}

#endif /* QUERYSTATE_HPP */

