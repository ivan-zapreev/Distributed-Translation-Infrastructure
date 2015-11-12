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

#include <functional>   //std::function
#include <string>       //std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"
#include "GenericTrieBase.hpp"

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
                : m_trie(trie), m_gram(trie.get_word_index()) {
                }

                /**
                 * Tokenise a given piece of text into a space separated list of text pieces.
                 * @param text the piece of text to tokenise
                 * @param gram the gram container to put data into
                 */
                inline void set_m_gram_from_text(TextPieceReader &text) {
                    m_gram.set_m_gram_from_text(text);
                }

            protected:
                //Stores the reference to the constant trie.
                const TrieType & m_trie;

                //Stores the query m-gram
                T_Query_M_Gram<WordIndexType, MAX_LEVEL> m_gram;

                //Stores pointers to the retrieved payloads
                void * m_payloads[MAX_LEVEL][MAX_LEVEL];

                //Stores the computed conditional probabilities per sub-m-gram
                TLogProbBackOff m_probs[MAX_LEVEL];
            };
        }
    }
}

#endif	/* QUERYSTATE_HPP */

