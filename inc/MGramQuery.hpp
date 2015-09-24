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
using namespace uva::smt::tries::mgrams;
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
                TLogProbBackOff prob;
            } TQueryResult;

            /**
             * Stores the query and its internal for the sake of re-usability and
             * independency from the Tries and executor.
             */
            template<TModelLevel N, typename WordIndexType>
            struct MGramQuery {
                //Stores the unknown word masks for the probability computations,
                //up to and including 8-grams:
                // 00000000, 00000001, 00000011, 00000111, 00001111,
                // 00011111, 00111111, 01111111, 11111111
                static const uint8_t PROB_UNK_MASKS[];

                //Stores the unknown word masks for the back-off weight computations,
                //up to and including 8-grams:
                // 00000000, 00000010, 00000110, 00001110,
                // 00011110, 00111110, 01111110, 11111110
                static const uint8_t BACK_OFF_UNK_MASKS[];

                //Unknown word bits
                uint8_t m_unk_word_flags = 0;

                //The temporary data structure to store the N-gram word ids
                TShortId m_query_word_ids[N] = {};

                //Stores the query m-gram
                T_M_Gram m_gram;

                //Stores the reference to the word index to be used
                const WordIndexType & m_word_index;

                //Stores the query result
                TQueryResult result = {};

                //Stores the current query level during the query execution
                TModelLevel curr_level = M_GRAM_LEVEL_UNDEF;

                //Stores the current end word index during the query execution
                TModelLevel curr_end_word_idx = 0;

                /**
                 * The basic constructor for the structure
                 * @param word_index the word index reference to store
                 */
                MGramQuery(WordIndexType & word_index) : m_word_index(word_index) {
                }

                /**
                 * Allows t set a new query into this state object
                 * @param m_gram the query M-gram
                 */
                inline void prepare_query() {
                    //Check the number of elements in the N-Gram
                    if (DO_SANITY_CHECKS && ((m_gram.level < M_GRAM_LEVEL_1) || (m_gram.level > N))) {
                        stringstream msg;
                        msg << "An improper N-Gram size, got " << m_gram.level << ", must be between [1, " << N << "]!";
                        throw Exception(msg.str());
                    } else {
                        //Set the currently considered M-gram level
                        curr_level = m_gram.level;

                        //Set the result probability to zero
                        result.prob = ZERO_PROB_WEIGHT;

                        //Transform the given M-gram into word hashes.
                        m_gram.store_m_gram_word_ids<N, WordIndexType>(m_query_word_ids, m_word_index);

                        //Store unknown word flags
                        store_unk_word_flags();
                    }
                }

                /**
                 * Gets the word hash for the end word of the back-off M-Gram
                 * @return the word hash for the end word of the back-off M-Gram
                 */
                inline const TShortId & get_back_off_end_word_id() {
                    //The word ids are always aligned to the end of the array
                    //so the end word id for the back off m-gram is fixed!
                    return m_query_word_ids[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the M-gram
                 * @return the word hash for the last word in the M-gram
                 */
                inline const TShortId & get_end_word_id() {
                    //The word ids are always aligned to the end of the array
                    //so the end word id for the probability m-gram is fixed!
                    return m_query_word_ids[N - 1];
                }

                /**
                 * Has to be called after the method that stores the query word ids.
                 * This one looks at the query word ids and creates binary flag map
                 * of the unknown word ids present in the current M-gram
                 */
                inline void store_unk_word_flags() {
                    const TModelLevel max_supp_level = (sizeof (PROB_UNK_MASKS) - 1);

                    if (DO_SANITY_CHECKS && (N > max_supp_level)) {
                        stringstream msg;
                        msg << "store_unk_word_flags: Unsupported m-gram level: "
                                << SSTR(N) << ", must be <= " << SSTR(max_supp_level)
                                << "], insufficient m_unk_word_flags capacity!";
                        throw Exception(msg.str());
                    }

                    //Re-initialize the flags with zero
                    m_unk_word_flags = 0;
                    //Fill in the values from the currently considered M-gram word ids
                    for (size_t idx = (N - m_gram.level); idx < N; ++idx) {
                        if (m_query_word_ids[idx] == AWordIndex::UNKNOWN_WORD_ID) {
                            m_unk_word_flags |= (1u << ((N - 1) - idx));
                        }
                    }

                    LOG_DEBUG << "The query unknown word flags are: "
                            << bitset<NUM_BITS_IN_UINT_8>(m_unk_word_flags) << END_LOG;
                }

                /**
                 * Allows to check if the given back-off sub-m-gram contains 
                 * an unknown word for the given current level.
                 */
                template<bool is_back_off>
                bool has_no_unk_words() const {
                    uint8_t level_flags = (m_unk_word_flags & ((is_back_off) ? BACK_OFF_UNK_MASKS[curr_level] : PROB_UNK_MASKS[curr_level]));

                    LOG_DEBUG << "The " << ((is_back_off) ? "back-off" : "probability")
                            << " level: " << curr_level << " unknown word flags are: "
                            << bitset<NUM_BITS_IN_UINT_8>(level_flags) << END_LOG;

                    return (level_flags == 0);
                }
            };

            template<TModelLevel N, typename WordIndexType>
            const uint8_t MGramQuery<N, WordIndexType>::PROB_UNK_MASKS[] = {
                0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
            };

            template<TModelLevel N, typename WordIndexType>
            const uint8_t MGramQuery<N, WordIndexType>::BACK_OFF_UNK_MASKS[] = {
                0x00, 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template struct MGramQuery<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template struct MGramQuery<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template struct MGramQuery<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template struct MGramQuery<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;

        }
    }
}

#endif	/* QUERYSTATE_HPP */

