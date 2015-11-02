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

#include "MGrams.hpp"
#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"

#include "AWordIndex.hpp"
#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"
#include "LayeredTrieDriver.hpp"
#include "G2DHashMapTrie.hpp"
#include "GenericTrieDriver.hpp"

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
            template<typename TrieType, bool IS_CUMULATIVE_PROB>
            class T_M_Gram_Query {
            public:
                typedef typename TrieType::WordIndexType WordIndexType;

                //Define the maximum level constant
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;

                //Define the the number of elements in the probability and back of arrays. We 
                //use one extra element in each to store the pre-fetches value for the <unk> word.
                static constexpr TModelLevel NUM_PROB_BACK_ELEMS = MAX_LEVEL + 1;

                //The index of the first m-gram word index
                static constexpr TModelLevel FIRST_WORD_IDX = 0;
                //The index of the last m-gram word index
                static constexpr TModelLevel LAST_WORD_IDX = MAX_LEVEL - 1;
                //The index of the unknown word
                static constexpr TModelLevel UNKNOWN_WORD_IDX = NUM_PROB_BACK_ELEMS - 1;

                //The index of the first sub-m-gram
                static constexpr TModelLevel FIRST_SUB_M_GRAM_IDX = 0;
                //The index of the last sub-m-gram
                static constexpr TModelLevel LAST_SUB_M_GRAM_IDX = MAX_LEVEL - 1;

                /**
                 * The basic constructor for the structure
                 * @param trie the reference to the trie object
                 */
                T_M_Gram_Query(TrieType & trie) : m_trie(trie), m_gram(trie.get_word_index()), m_gram_old(trie.get_word_index()) {
                    m_trie.get_unk_word_payload(m_prob[UNKNOWN_WORD_IDX], m_back[UNKNOWN_WORD_IDX]);
                }

                /**
                 * Tokenise a given piece of text into a space separated list of text pieces.
                 * @param text the piece of text to tokenise
                 * @param gram the gram container to put data into
                 */
                inline void set_m_gram_from_text(TextPieceReader &text) {
                    m_gram_old.set_m_gram_from_text(text);
                }

                /**
                 * Allows to log the query results after its execution.
                 * Different logging is done based on enabled logging level
                 * and the class template parameters.
                 */
                inline void log_results() const {
                    //Initialize the current index, with the proper start value
                    TModelLevel curr_idx = (IS_CUMULATIVE_PROB ? FIRST_SUB_M_GRAM_IDX : LAST_SUB_M_GRAM_IDX);
                    TLogProbBackOff cumulative_prob = ZERO_PROB_WEIGHT;

                    //Print the intermediate results
                    for (; curr_idx <= LAST_SUB_M_GRAM_IDX; ++curr_idx) {
                        const string gram_str = m_gram_old.get_mgram_prob_str(curr_idx + 1);
                        LOG_RESULT << "log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                << " ) ) = " << SSTR(m_prob[curr_idx]) << END_LOG;
                        LOG_INFO << "Prob( " << gram_str << " ) = "
                                << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_prob[curr_idx])) << END_LOG;
                        if (m_prob[curr_idx] > ZERO_LOG_PROB_WEIGHT) {
                            cumulative_prob += m_prob[curr_idx];
                        }
                    }

                    //Print the total cumulative probability if needed
                    if (IS_CUMULATIVE_PROB) {
                        const string gram_str = m_gram_old.get_mgram_prob_str();
                        LOG_RESULT << "log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                << " ) ) = " << SSTR(cumulative_prob) << END_LOG;
                        LOG_INFO << "Prob( " << gram_str << " ) = "
                                << SSTR(pow(LOG_PROB_WEIGHT_BASE, cumulative_prob)) << END_LOG;
                    }
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param gram the given M-Gram query and its state
                 * @param result the structure to store the resulting probability
                 */
                void execute_new() {
                    LOG_DEBUG << "Starting to execute:" << (string) m_gram_old << END_LOG;

                    //Prepare the data structures for begin used, do not clean the last elements as
                    //they store the pre-fetched prob and back off values for the <unk> word.
                    memset(m_prob, ZERO_PROB_WEIGHT, (NUM_PROB_BACK_ELEMS - 1) * sizeof (TLogProbBackOff));
                    memset(m_back, ZERO_PROB_WEIGHT, (NUM_PROB_BACK_ELEMS - 1) * sizeof (TLogProbBackOff));

                    //Prepare the m-gram for querying
                    m_gram.prepare_for_querying();

                    //Define and initialize the first considered word index
                    TModelLevel begin_word_idx = FIRST_WORD_IDX;
                    //Define and initialize the last considered word index
                    TModelLevel end_word_idx = (IS_CUMULATIVE_PROB ? FIRST_WORD_IDX : LAST_WORD_IDX);

                    //Iterate through the model and compute probabilities: going right in the row
                    for (; end_word_idx <= LAST_WORD_IDX; ++end_word_idx) {
                        //Try to get the probability of the sub-m-gram, if it is not there: back-off
                        if (m_add_prob_get_back_off[begin_word_idx][end_word_idx](m_trie, m_gram, m_prob, m_back)) {
                            //Compute the back-off sub-m-gram end word index
                            const TModelLevel bo_end_word_idx = (end_word_idx - 1);
                            //Add the back off weight from the previous computation, always available if not the first unigram
                            if (end_word_idx > FIRST_WORD_IDX) {
                                //Get the stored back-off from the previous level
                                m_prob[end_word_idx] = m_back[bo_end_word_idx];
                            }

                            //Now continue the back-off process: going down the column
                            while (m_add_prob_get_back_off[++begin_word_idx][end_word_idx](m_trie, m_gram, m_prob, m_back)) {
                                //Get the back-off weight of the previous level
                                m_add_back_off[begin_word_idx][bo_end_word_idx](m_trie, m_gram, m_prob, m_back);
                            };
                        }
                    }
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param gram the given M-Gram query and its state
                 * @param result the structure to store the resulting probability
                 */
                void execute() {
                    LOG_DEBUG << "Starting to execute:" << (string) m_gram_old << END_LOG;

                    //Set the result probability to zero
                    m_prob[LAST_SUB_M_GRAM_IDX] = ZERO_PROB_WEIGHT;

                    //Prepare the m-gram for querying
                    m_gram_old.prepare_for_querying();

                    //Get the used level and go on with execution
                    TModelLevel curr_level = m_gram_old.m_actual_level;

                    //Compute the probability in the loop fashion, should be faster that recursion.
                    while (m_prob[LAST_SUB_M_GRAM_IDX] == ZERO_PROB_WEIGHT) {
                        //Do a sanity check if needed
                        if (DO_SANITY_CHECKS && (curr_level < M_GRAM_LEVEL_1)) {
                            stringstream msg;
                            msg << "An impossible value of curr_level: " << SSTR(curr_level)
                                    << ", it must be >= " << SSTR(M_GRAM_LEVEL_1);
                            throw Exception(msg.str());
                        }

                        //Try to compute the next probability with decreased level
                        m_trie.get_prob_weight(curr_level, m_gram_old, m_prob[LAST_SUB_M_GRAM_IDX]);

                        //Decrease the level
                        curr_level--;
                    }

                    LOG_DEBUG << "The current level value is: " << curr_level
                            << ", the current probability value is: " << m_prob[LAST_SUB_M_GRAM_IDX] << END_LOG;

                    //If the probability is log-zero or snaller then there is no
                    //need for a back-off as then we will only get smaller values.
                    if (m_prob[LAST_SUB_M_GRAM_IDX] > ZERO_LOG_PROB_WEIGHT) {
                        //If the curr_level is smaller than the original level then
                        //it means that we needed to back-off, add back-off weights
                        for (++curr_level; curr_level != m_gram_old.m_actual_level; ++curr_level) {
                            //Get the back_off 
                            m_trie.add_back_off_weight(curr_level, m_gram_old, m_prob[LAST_SUB_M_GRAM_IDX]);
                        }
                    }

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE
                            << " probability is: " << m_prob[LAST_SUB_M_GRAM_IDX] << END_LOG;
                }

            private:
                //Stores the reference to the constant trie.
                const TrieType & m_trie;

                //Stores the query m-gram
                T_Query_M_Gram<WordIndexType, MAX_LEVEL> m_gram;

                //Stores the probability results for the sub-m-grams, add
                //an extra element for the pre-fetched unknown word data
                TLogProbBackOff m_prob[MAX_LEVEL + 1];

                //Stores the back-off weights for the sub-m-grams, add
                //an extra element for the pre-fetched unknown word data
                TLogProbBackOff m_back[MAX_LEVEL + 1];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<bool (const TrieType&, const T_Query_M_Gram<WordIndexType> &,
                        TLogProbBackOff[NUM_PROB_BACK_ELEMS], TLogProbBackOff[NUM_PROB_BACK_ELEMS]) > TAddProbGetBackFunc;

                //Stores the get-payload function pointers for getting probabilities and back offs
                static const TAddProbGetBackFunc m_add_prob_get_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7];

                //The typedef for the function that gets the payload from the trie
                typedef std::function<void (const TrieType&, const T_Query_M_Gram<WordIndexType> &,
                        TLogProbBackOff[NUM_PROB_BACK_ELEMS], TLogProbBackOff[NUM_PROB_BACK_ELEMS]) > TAddBackOffFunc;

                //Stores the get-payload function pointers for getting back-offs
                static const TAddBackOffFunc m_add_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7];

                /**
                 * Depending on the value of the IS_PROB template parameter we have two different behaviors:
                 * A) IS_PROB == true
                 *    if( m-gram found ) {
                 *      prob += stored_prob
                 *      back  = stored_back
                 *    } else {
                 *      //nothing
                 *    }
                 * B) IS_PROB == false
                 *    if( m-gram found ) {
                 *      prob += stored_back
                 *    } else {
                 *      //nothing
                 *    }
                 * @param trie
                 * @param gram
                 * @param prob
                 * @param back
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                static inline bool add_prob_get_back_off(const TrieType& trie, const T_Query_M_Gram<WordIndexType> & gram,
                        TLogProbBackOff prob[NUM_PROB_BACK_ELEMS], TLogProbBackOff back[NUM_PROB_BACK_ELEMS]) {
                    //ToDo: We do not want to check if the begin word is unknown! we want to jump the diagonal if needed!

                    //Check if the begin or end word is unknown
                    if ((gram[BEGIN_WORD_IDX] == WordIndexType::UNKNOWN_WORD_ID)
                            || (gram[END_WORD_IDX] == WordIndexType::UNKNOWN_WORD_ID)) {
                        //If true then the payload is unavailable unless this is an unknown word unigram
                        if (BEGIN_WORD_IDX == END_WORD_IDX) {
                            prob[END_WORD_IDX] += prob[UNKNOWN_WORD_IDX];
                            back[END_WORD_IDX] = back[UNKNOWN_WORD_IDX];
                            //we got the data for the unknown word, so return true
                            return true;
                        } else {
                            //The data is not to be found, so return false
                            return false;
                        }
                    } else {
                        //Retrieve the payload from the trie
                        return trie.template add_payload<BEGIN_WORD_IDX, END_WORD_IDX>(gram, prob[END_WORD_IDX], back[END_WORD_IDX]);
                    }
                };

                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                static inline void add_back_off(const TrieType& trie, const T_Query_M_Gram<WordIndexType> & gram,
                        TLogProbBackOff prob[NUM_PROB_BACK_ELEMS], TLogProbBackOff back[NUM_PROB_BACK_ELEMS]) {
                    //Compute the end word index for the back-off m-gram
                    constexpr TModelLevel BACK_OFF_END_WORD_IDX = END_WORD_IDX - 1;

                    //ToDo: We do not want to check if the begin word is unknown! we want to jump the diagonal if needed!

                    //Check if the begin or end word is unknown
                    if ((gram[BEGIN_WORD_IDX] == WordIndexType::UNKNOWN_WORD_ID)
                            || (gram[BACK_OFF_END_WORD_IDX] == WordIndexType::UNKNOWN_WORD_ID)) {
                        //If true then the payload is unavailable unless this is an unknown word unigram
                        if (BEGIN_WORD_IDX == BACK_OFF_END_WORD_IDX) {
                            //Add he <unk> word back-off weight
                            prob[END_WORD_IDX] += back[UNKNOWN_WORD_IDX];
                        } else {
                            //The data is not to be found, so the back-off is zero
                        }
                    } else {
                        //Define the dummy probability variable, who's value will not be used
                        TLogProbBackOff dummy_prob;
                        //Add the back-off weight to the probability
                        (void) trie.template add_payload<BEGIN_WORD_IDX, BACK_OFF_END_WORD_IDX>(gram, dummy_prob, prob[END_WORD_IDX]);
                    }
                }

                //Stores the query m-gram
                T_M_Gram<WordIndexType> m_gram_old;
            };

            template<typename TrieType, bool IS_CUMULATIVE_PROB>
            const typename T_M_Gram_Query<TrieType, IS_CUMULATIVE_PROB>::TAddProbGetBackFunc T_M_Gram_Query<TrieType, IS_CUMULATIVE_PROB>::m_add_prob_get_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7] = {
                {&add_prob_get_back_off<0, 0>, &add_prob_get_back_off<0, 1>, &add_prob_get_back_off<0, 2>, &add_prob_get_back_off<0, 3>, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, &add_prob_get_back_off<0, 1>, &add_prob_get_back_off<0, 2>, &add_prob_get_back_off<0, 3>, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, NULL, &add_prob_get_back_off<0, 2>, &add_prob_get_back_off<0, 3>, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, NULL, NULL, &add_prob_get_back_off<0, 3>, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, &add_prob_get_back_off<0, 4>, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, NULL, &add_prob_get_back_off<0, 5>, &add_prob_get_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &add_prob_get_back_off<0, 6>}
            };

            template<typename TrieType, bool IS_CUMULATIVE_PROB>
            const typename T_M_Gram_Query<TrieType, IS_CUMULATIVE_PROB>::TAddBackOffFunc T_M_Gram_Query<TrieType, IS_CUMULATIVE_PROB>::m_add_back_off[M_GRAM_LEVEL_7][M_GRAM_LEVEL_7] = {
                {&add_back_off<0, 0>, &add_back_off<0, 1>, &add_back_off<0, 2>, &add_back_off<0, 3>, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, &add_back_off<0, 1>, &add_back_off<0, 2>, &add_back_off<0, 3>, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, NULL, &add_back_off<0, 2>, &add_back_off<0, 3>, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, NULL, NULL, &add_back_off<0, 3>, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, &add_back_off<0, 4>, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, NULL, &add_back_off<0, 5>, &add_back_off<0, 6>},
                {NULL, NULL, NULL, NULL, NULL, NULL, &add_back_off<0, 6>}
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX_IS_CUM_PROB(M_GRAM_LEVEL, WORD_INDEX_TYPE, IS_CUM_PROB); \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>, IS_CUM_PROB>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>, IS_CUM_PROB>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>, IS_CUM_PROB>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>, IS_CUM_PROB>; \
            template class T_M_Gram_Query<GenericTrieDriver<LayeredTrieDriver<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>, IS_CUM_PROB>; \
            template class T_M_Gram_Query<GenericTrieDriver<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>, IS_CUM_PROB>;

#define INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_IS_CUM_PROB(M_GRAM_LEVEL, IS_CUM_PROB); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX_IS_CUM_PROB(M_GRAM_LEVEL, BasicWordIndex, IS_CUM_PROB); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX_IS_CUM_PROB(M_GRAM_LEVEL, CountingWordIndex, IS_CUM_PROB); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX_IS_CUM_PROB(M_GRAM_LEVEL, TOptBasicWordIndex, IS_CUM_PROB); \
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_WORD_IDX_IS_CUM_PROB(M_GRAM_LEVEL, TOptCountWordIndex, IS_CUM_PROB);

            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_IS_CUM_PROB(M_GRAM_LEVEL_MAX, true);
            INSTANTIATE_TYPEDEF_M_GRAM_QUERIES_LEVEL_IS_CUM_PROB(M_GRAM_LEVEL_MAX, false);
        }
    }
}

#endif	/* QUERYSTATE_HPP */

