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
                    //Check that the level is supported
                    if(MAX_LEVEL > M_GRAM_LEVEL_7) {
                        stringstream msg;
                        msg << "The T_M_Gram_Query class in " << __FILE__ << " does not support "
                                << "trie level: " << SSTR(MAX_LEVEL) << ", the maximum supported "
                                << "level is: " << SSTR(M_GRAM_LEVEL_7) << ", please extend!";
                        throw Exception(msg.str());
                    }
                    
                    m_trie.get_unk_word_payload(m_unk_word_data);
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

                //Stores the unknown word payload data
                T_M_Gram_Payload m_unk_word_data;

                //Stores the retrieved sub-m-gram payloads
                T_M_Gram_Payload m_payload[MAX_LEVEL][MAX_LEVEL];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<bool (const TrieType&, const T_Query_M_Gram<WordIndexType> &,
                        T_M_Gram_Payload[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff &) > TAddProbGetBackFunc;

                //Stores the get-payload function pointers for getting probabilities and back offs
                static const TAddProbGetBackFunc m_add_prob_get_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<void (const TrieType&, const T_Query_M_Gram<WordIndexType> &,
                        T_M_Gram_Payload[MAX_LEVEL][MAX_LEVEL], TLogProbBackOff &) > TAddBackOffFunc;

                //Stores the get-payload function pointers for getting back-offs
                static const TAddBackOffFunc m_add_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7];

                /**
                 * Retrieves the payload of the sub-m-gram defined by the begin
                 * and end word indexes and increments with provided probability
                 * variable, via reference, with the obtained probability payload.
                 * @param trie the trie to work with
                 * @param gram the complete m-gram 
                 * @param payload the payload structures for storing retrieved playload
                 * @param prob the reference to the probability variable to be incremented
                 * @return true if the payload was not found, otherwise false
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                static inline bool add_prob_get_back_off(const TrieType& trie,
                        const T_Query_M_Gram<WordIndexType> & gram,
                        T_M_Gram_Payload payload[MAX_LEVEL][MAX_LEVEL],
                        TLogProbBackOff & prob) {
                    //Store the reference to the payload data
                    T_M_Gram_Payload & data = payload[BEGIN_WORD_IDX][END_WORD_IDX];

                    //Retrieve the payload from the trie
                    if (trie.template get_payload<BEGIN_WORD_IDX, END_WORD_IDX>(gram, data)) {
                        LOG_DEBUG1 << "Adding the probability from [" << SSTR(BEGIN_WORD_IDX) << ", "
                                << SSTR(END_WORD_IDX) << "] = " << data.prob << END_LOG;

                        //If the payload was found then add add it to the probability
                        prob += data.prob;
                        //False indicates that the payload was found
                        return false;
                    } else {
                        LOG_DEBUG1 << "Adding the probability from [" << SSTR(BEGIN_WORD_IDX) << ", "
                                << SSTR(END_WORD_IDX) << "] = ZERO" << END_LOG;

                        //True indicates that we need to back-off
                        return true;
                    }
                };

                /**
                 * Retrieves the payload of the sub-m-gram defined by the begin
                 * and end word indexes and increments with provided probability
                 * variable, via reference, with the obtained back-off payload.
                 * @param trie the trie to work with
                 * @param gram the complete m-gram 
                 * @param payload the payload structures for storing retrieved playload
                 * @param prob the reference to the probability variable to be incremented
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                static inline void add_back_off(const TrieType& trie,
                        const T_Query_M_Gram<WordIndexType> & gram,
                        T_M_Gram_Payload payload[MAX_LEVEL][MAX_LEVEL],
                        TLogProbBackOff & prob) {
                    //Store the reference to the payload data
                    T_M_Gram_Payload & data = payload[BEGIN_WORD_IDX][END_WORD_IDX];

                    //Retrieve the payload from the trie
                    if (trie.template get_payload<BEGIN_WORD_IDX, END_WORD_IDX>(gram, data)) {
                        LOG_DEBUG1 << "Adding the back-off from [" << SSTR(BEGIN_WORD_IDX) << ", "
                                << SSTR(END_WORD_IDX) << "] = " << data.back << END_LOG;

                        //If the payload was found then add the back-off weight, otherwise it is zero
                        prob += data.back;
                    } else {
                        LOG_DEBUG1 << "Adding the back-off from [" << SSTR(BEGIN_WORD_IDX) << ", "
                                << SSTR(END_WORD_IDX) << "] = ZERO" << END_LOG;
                    }
                }
            };

            template<typename TrieType>
            const typename T_M_Gram_Query<TrieType>::TAddProbGetBackFunc T_M_Gram_Query<TrieType>::m_add_prob_get_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7] = {
                {&add_prob_get_back_off<0, 0>, &add_prob_get_back_off<0, 1>, &add_prob_get_back_off<0, 2>, &add_prob_get_back_off<0, 3>, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, &add_prob_get_back_off<1, 1>, &add_prob_get_back_off<1, 2>, &add_prob_get_back_off<1, 3>, &add_prob_get_back_off<1, 4>, &add_prob_get_back_off<1, 5>, &add_prob_get_back_off<1, 6>},
                {NULL, NULL, &add_prob_get_back_off<2, 2>, &add_prob_get_back_off<2, 3>, &add_prob_get_back_off<2, 4>, &add_prob_get_back_off<2, 5>, &add_prob_get_back_off<2, 6>},
                {NULL, NULL, NULL, &add_prob_get_back_off<3, 3>, &add_prob_get_back_off<3, 4>, &add_prob_get_back_off<3, 5>, &add_prob_get_back_off<3, 6>},
                {NULL, NULL, NULL, NULL, &add_prob_get_back_off<4, 4>, &add_prob_get_back_off<4, 5>, &add_prob_get_back_off<4, 6>},
                {NULL, NULL, NULL, NULL, NULL, &add_prob_get_back_off<5, 5>, &add_prob_get_back_off<5, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &add_prob_get_back_off<6, 6>}
            };

            template<typename TrieType>
            const typename T_M_Gram_Query<TrieType>::TAddBackOffFunc T_M_Gram_Query<TrieType>::m_add_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7] = {
                {&add_back_off<0, 0>, &add_back_off<0, 1>, &add_back_off<0, 2>, &add_back_off<0, 3>, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, &add_back_off<1, 1>, &add_back_off<1, 2>, &add_back_off<1, 3>, &add_back_off<1, 4>, &add_back_off<1, 5>, &add_back_off<1, 6>},
                {NULL, NULL, &add_back_off<2, 2>, &add_back_off<2, 3>, &add_back_off<2, 4>, &add_back_off<2, 5>, &add_back_off<2, 6>},
                {NULL, NULL, NULL, &add_back_off<3, 3>, &add_back_off<3, 4>, &add_back_off<3, 5>, &add_back_off<3, 6>},
                {NULL, NULL, NULL, NULL, &add_back_off<4, 4>, &add_back_off<4, 5>, &add_back_off<4, 6>},
                {NULL, NULL, NULL, NULL, NULL, &add_back_off<5, 5>, &add_back_off<5, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &add_back_off<6, 6>}
            };
        }
    }
}

#endif	/* QUERYSTATE_HPP */

