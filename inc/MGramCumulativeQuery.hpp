/* 
 * File:   MGramCumulativeQuery.hpp
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

#ifndef MGRAMCUMULATIVEQUERY_HPP
#define	MGRAMCUMULATIVEQUERY_HPP

#include <string>       //std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"
#include "MGramQuery.hpp"

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
             *      \Sum_{1}^{5}log_10(Prob(w_{i}|w_{1}...w_{i-1}))
             * where log_10(Prob(w_{i}|w_{1}...w_{i-1})) is > ZERO_LOG_PROB_WEIGHT
             * 
             * Note that, here 5 is taken just as an example.
             */
            template<typename TrieType>
            class T_M_Gram_Cumulative_Query : public T_M_Gram_Query<TrieType> {
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
                T_M_Gram_Cumulative_Query(TrieType & trie) : T_M_Gram_Query<TrieType>(trie) {
                }

                /**
                 * Allows to log the query results after its execution.
                 * Different logging is done based on enabled logging level
                 * and the class template parameters.
                 */
                inline void log_results() const {
                    //Initialize the current index, with the proper start value
                    TModelLevel curr_idx = BASE::m_gram.get_begin_word_idx();
                    TLogProbBackOff cumulative_prob = ZERO_PROB_WEIGHT;

                    //Print the intermediate results
                    for (; curr_idx <= BASE::m_gram.get_end_word_idx(); ++curr_idx) {
                        const string gram_str = BASE::m_gram.get_mgram_prob_str(curr_idx + 1);
                        LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                                << " ) ) = " << SSTR(m_prob[curr_idx]) << END_LOG;
                        LOG_INFO << "  Prob( " << gram_str << " ) = "
                                << SSTR(pow(LOG_PROB_WEIGHT_BASE, m_prob[curr_idx])) << END_LOG;
                        if (m_prob[curr_idx] > ZERO_LOG_PROB_WEIGHT) {
                            cumulative_prob += m_prob[curr_idx];
                        }
                        LOG_RESULT << "---" << END_LOG;
                    }

                    //Print the total cumulative probability if needed
                    const string gram_str = BASE::m_gram.get_mgram_prob_str();
                    LOG_RESULT << "  log_" << LOG_PROB_WEIGHT_BASE << "( Prob( " << gram_str
                            << " ) ) = " << SSTR(cumulative_prob) << END_LOG;
                    LOG_INFO << "  Prob( " << gram_str << " ) = "
                            << SSTR(pow(LOG_PROB_WEIGHT_BASE, cumulative_prob)) << END_LOG;

                    LOG_RESULT << "-------------------------------------------" << END_LOG;
                }

                /**
                 * Allows to execute m-gram the query
                 */
                void execute() {
                    LOG_DEBUG << "Starting to execute:" << (string) BASE::m_gram << END_LOG;

                    //Prepare the m-gram for querying
                    BASE::m_gram.template prepare_for_querying<false>();

                    //Initialize the begin and end index variables
                    TModelLevel begin_word_idx = BASE::m_gram.get_begin_word_idx();
                    TModelLevel end_word_idx = BASE::m_gram.get_begin_word_idx();

                    //Clean the sub-m-gram probability array
                    memset(m_prob, ZERO_PROB_WEIGHT, MAX_LEVEL * sizeof (TLogProbBackOff));

                    //Iterate through sub-m-grams: going right through the row
                    for (; end_word_idx <= BASE::m_gram.get_end_word_idx(); ++end_word_idx) {
                        LOG_DEBUG << "-----> Considering cumulative sub-m-gram [" << SSTR(begin_word_idx)
                                << ", " << SSTR(end_word_idx) << "]" << END_LOG;
                        if (BASE::m_gram[end_word_idx] == WordIndexType::UNKNOWN_WORD_ID) {
                            //If the sub-m-gram's end word is unknown back-off
                            do_back_off_unknown(begin_word_idx, end_word_idx);
                        } else {
                            //If the sub-m-gram's word is known try to retrieve the payload
                            if (BASE::m_add_prob_get_back_off[begin_word_idx][end_word_idx](BASE::m_trie, BASE::m_gram, BASE::m_payload, m_prob[end_word_idx])) {
                                //If the sub-m-gram payload is not defined then back-off
                                do_back_off_undefined(begin_word_idx, end_word_idx);
                            }
                        }
                    }
                }

            private:

                //Stores the probability results for the sub-m-grams, add
                //an extra element for the pre-fetched unknown word data
                TLogProbBackOff m_prob[MAX_LEVEL];

                /**
                 * The last word of the m-gram is an unknown word, just assign use 
                 * the unknown word payload. We could have used back-offs and such 
                 * but there is no need as the probability will only get lower.
                 * @param begin_word_idx the index of the begin word defining this sub-m-gram
                 * @param end_word_idx the index of the end word defining this sub-m-gram
                 */
                inline void do_last_word_unknown(const TModelLevel begin_word_idx, const TModelLevel end_word_idx) {
                    LOG_DEBUG1 << "The last word (index: " << SSTR(end_word_idx)
                            << ") is unknonwn, using the <unk> word data!" << END_LOG;

                    //The unigram is an unknown word so copy the payload from <unk>
                    BASE::m_payload[end_word_idx][end_word_idx] = BASE::m_unk_word_data;

                    LOG_DEBUG1 << "Adding the <unk> word probability data for [" << SSTR(begin_word_idx)
                            << ", " << SSTR(end_word_idx) << "] = " << BASE::m_unk_word_data.prob << END_LOG;

                    //Add the <unk> word probability to the sub-m-gram probability
                    m_prob[end_word_idx] += BASE::m_unk_word_data.prob;
                }

                /**
                 * This back-off function is used in case that the sub-m-gram contains
                 * no unknown words but is still not present in the trie. Naturally this
                 * means that it is the case of a sub-m-gram with level > 1.
                 * @param begin_word_idx the index of the begin word defining this sub-m-gram
                 * @param end_word_idx the index of the end word defining this sub-m-gram
                 */
                inline void do_back_off_undefined(TModelLevel & begin_word_idx, const TModelLevel end_word_idx) {
                    LOG_DEBUG1 << "The sub-m-gram [" << SSTR(begin_word_idx) << ", " << SSTR(end_word_idx)
                            << "] was not found, need to back off!" << END_LOG;

                    //Perform sanity checks if needed
                    if (DO_SANITY_CHECKS && (begin_word_idx >= end_word_idx)) {
                        stringstream msg;
                        msg << "Improper begin/end index pair in do_back_off_undefined: "
                                << "begin_word_idx (" << SSTR(begin_word_idx)
                                << ") >= end_word_idx (" << SSTR(end_word_idx) << ")!";
                        throw Exception(msg.str());
                    }

                    //Define the index of the end word for the back-off n-gram
                    const TModelLevel bo_end_word_idx = end_word_idx - 1;

                    //Add the back-off weight from the previous sub-m-gram column. That should 
                    //exist, as the undefined case means we have "begin_word_idx < end_word_idx"
                    //If we are computing the cumulative query then the data must have been there
                    LOG_DEBUG1 << "Adding the back-off data from [" << SSTR(begin_word_idx)
                            << ", " << SSTR(bo_end_word_idx) << "] = "
                            << BASE::m_payload[begin_word_idx][bo_end_word_idx].back << END_LOG;

                    //Get the stored back-off from the previous level
                    m_prob[end_word_idx] += BASE::m_payload[begin_word_idx][bo_end_word_idx].back;

                    LOG_DEBUG1 << "Continue by going down the column, retrieve payload for ["
                            << SSTR(begin_word_idx + 1) << ", " << SSTR(end_word_idx) << "]" << END_LOG;

                    //Now continue the back-off process: going down the column
                    while (BASE::m_add_prob_get_back_off[++begin_word_idx][end_word_idx](BASE::m_trie, BASE::m_gram, BASE::m_payload, m_prob[end_word_idx])) {
                        LOG_DEBUG1 << "The payload probability for [" << SSTR(begin_word_idx) << ", "
                                << SSTR(end_word_idx) << "] was not found, doing back-off!" << END_LOG;
                        BASE::m_add_back_off[begin_word_idx][bo_end_word_idx](BASE::m_trie, BASE::m_gram, BASE::m_payload, m_prob[end_word_idx]);
                    };
                }

                /**
                 * This back-off function is used in case that the sub-m-gram's end word
                 * is unknown. Then the sub-m-gram is not to be found and this includes
                 * the case of a unigram consisting of an unknown word.
                 * @param begin_word_idx the index of the begin word defining this sub-m-gram
                 * @param end_word_idx the index of the end word defining this sub-m-gram
                 */
                inline void do_back_off_unknown(TModelLevel & begin_word_idx, const TModelLevel end_word_idx) {
                    //Define the index of the end word for the back-off n-gram
                    const TModelLevel bo_end_word_idx = end_word_idx - 1;

                    //If the begin and end index are different then we first need to go down the column
                    if (begin_word_idx != end_word_idx) {
                        LOG_DEBUG1 << "The last word (index: " << SSTR(end_word_idx)
                                << ") is unknown, going down from the word index: "
                                << SSTR(begin_word_idx) << " retrieving the back-off!" << END_LOG;

                        LOG_DEBUG1 << "Adding the back-off data from [" << SSTR(begin_word_idx)
                                << ", " << SSTR(bo_end_word_idx) << "] = "
                                << BASE::m_payload[begin_word_idx][bo_end_word_idx].back << END_LOG;

                        //Get the pre-fetched back-off weight from the previous level
                        m_prob[end_word_idx] += BASE::m_payload[begin_word_idx][bo_end_word_idx].back;

                        //Iterate down to the unigram, adding the back-offs on the way
                        while (++begin_word_idx != end_word_idx) {
                            //Get the back-off weight of the previous level
                            BASE::m_add_back_off[begin_word_idx][bo_end_word_idx](BASE::m_trie, BASE::m_gram, BASE::m_payload, m_prob[end_word_idx]);
                        }
                    }

                    //We are down to the last m-gram word that is unknown, use the <unk> payload
                    do_last_word_unknown(begin_word_idx, end_word_idx);

                    //For the efficiency reasons, if there is more sub-m-grams we need to go diagonal
                    if (end_word_idx != BASE::m_gram.get_end_word_idx()) {

                        LOG_DEBUG1 << "The last word (index: " << SSTR(end_word_idx)
                                << ") is unknown, but is not the actual end word: "
                                << SSTR(BASE::m_gram.get_end_word_idx())
                                << " so going diagonal!" << END_LOG;

                        LOG_DEBUG1 << "Adding the back-off data from [" << SSTR(begin_word_idx) << ", " << SSTR(end_word_idx)
                                << "] = " << BASE::m_payload[begin_word_idx][end_word_idx].back << END_LOG;

                        //Add the back-off weight from this row's unknown word
                        m_prob[end_word_idx + 1] += BASE::m_payload[begin_word_idx][end_word_idx].back;
                        //Shift to the next row as this row is from <unk>
                        begin_word_idx++;
                    }
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, WORD_INDEX_TYPE); \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<LayeredTrieDriver<C2DHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<LayeredTrieDriver<C2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<LayeredTrieDriver<C2WArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<LayeredTrieDriver<W2CArrayTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<LayeredTrieDriver<W2CHybridTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>>; \
            template class T_M_Gram_Cumulative_Query<GenericTrieDriver<G2DMapTrie<M_GRAM_LEVEL, WORD_INDEX_TYPE>>>;

#define INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, BasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, CountingWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptBasicWordIndex); \
            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL_WORD_IDX(M_GRAM_LEVEL, TOptCountWordIndex);

            INSTANTIATE_TYPEDEF_M_GRAM_CUMULATIVE_QUERY_LEVEL(M_GRAM_LEVEL_MAX);
        }
    }
}

#endif	/* MGRAMCUMULATIVEQUERY_HPP */

