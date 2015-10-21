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

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::smt::exceptions;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This data structure is to be used to return the N-Gram query result.
             * It contains the computed Back-Off language model probability and
             * potentially additional meta data for the decoder
             * @param prob the computed Back-Off language model probability as log_${LOG_PROB_WEIGHT_BASE}
             */
            typedef struct {
                TLogProbBackOff m_prob;
            } TQueryResult;

            /**
             * Stores the query and its internal for the sake of re-usability and
             * independency from the Tries and executor.
             */
            template<typename WordIndexType>
            struct MGramQuery {
                //Stores the query m-gram
                T_M_Gram<WordIndexType> m_gram;

                //Stores the query result
                TQueryResult m_result = {};

                //Stores the current end word index during the query execution
                TModelLevel m_curr_end_word_idx = 0;

                /**
                 * The basic constructor for the structure
                 * @param word_index the word index reference to store
                 */
                MGramQuery(WordIndexType & word_index) : m_gram(word_index) {
                }

                /**
                 * Allows t set a new query into this state object
                 * @param m_gram the query M-gram
                 * @return the m-gram's level
                 */
                inline TModelLevel prepare_query() {
                    //Set the result probability to zero
                    m_result.m_prob = ZERO_PROB_WEIGHT;

                    //Prepare the m-gram for querying
                    m_gram.prepare_for_querying();

                    //return the m-gram level
                    return m_gram.m_used_level;
                }

                /**
                 * Gets the word hash for the end word of the back-off M-Gram
                 * @return the word hash for the end word of the back-off M-Gram
                 */
                inline TShortId get_back_off_end_word_id() const {
                    return m_gram.get_back_off_end_word_id();
                }

                /**
                 * Gets the word hash for the last word in the M-gram
                 * @return the word hash for the last word in the M-gram
                 */
                inline TShortId get_end_word_id() const {
                    return m_gram.get_end_word_id();
                }

                /**
                 * Allows to check if the given back-off sub-m-gram contains 
                 * an unknown word for the given current level.
                 */
                template<bool is_back_off, TModelLevel curr_level>
                inline bool has_no_unk_words() const {
                    return m_gram.template has_no_unk_words<is_back_off, curr_level>();
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template struct MGramQuery<BasicWordIndex>;
            template struct MGramQuery<CountingWordIndex>;
            template struct MGramQuery<TOptBasicWordIndex>;
            template struct MGramQuery<TOptCountWordIndex>;

            typedef MGramQuery<BasicWordIndex> TMGramQueryBasic;
            typedef MGramQuery<CountingWordIndex> TMGramQueryCount;
            typedef MGramQuery<TOptBasicWordIndex> TMGramQueryOptBasic;
            typedef MGramQuery<TOptCountWordIndex> TMGramQueryOptCount;
        }
    }
}

#endif	/* QUERYSTATE_HPP */

