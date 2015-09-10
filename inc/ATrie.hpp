/* 
 * File:   ATrie.hpp
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
 * Created on September 3, 2015, 2:59 PM
 */

#ifndef ATRIE_HPP
#define	ATRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "MGrams.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::mgrams;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This structure is used to define the trivial probability/
             * back-off pari to be stored for M-grams with 1 <= M < N
             * @param prob stores the probability
             * @param back_off stores the back-off
             */
            typedef struct {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
            } TProbBackOffEntry;

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
             * This is a common abstract class for a Trie implementation.
             * The purpose of having this as a template class is performance optimization.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N>
            class ATrie {
            public:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = N - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit ATrie(AWordIndex * _pWordIndex)
                : m_p_word_index(_pWordIndex) {
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void pre_allocate(const size_t counts[N]) {
                    if (m_p_word_index != NULL) {
                        m_p_word_index->reserve(counts[0]);
                    }
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param oGram the 1-Gram data
                 */
                virtual void add_1_gram(const T_M_Gram &oGram) = 0;

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param mGram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void add_m_gram(const T_M_Gram &mGram) = 0;

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param nGram the N-Gram data
                 */
                virtual void add_n_gram(const T_M_Gram &nGram) = 0;

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * @param level the level of the X-grams that were finished to be read
                 */
                virtual bool is_post_grams(const TModelLevel level) {
                    return false;
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * @param level the level of the X-grams that were finished to be read
                 */
                void post_grams(const TModelLevel level) {
                    switch (level) {
                        case M_GRAM_LEVEL_1:
                            this->post_1_grams();
                            break;
                        case N:
                            this->post_n_grams();
                            break;
                        default:
                            this->post_m_grams(level);
                            break;
                    }
                };

                /**
                 * Returns the maximum length of the considered N-Grams
                 * @return the maximum length of the considered N-Grams
                 */
                TModelLevel get_max_level() const {
                    return N;
                }

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * @param ngram the given M-Gram we are going to query!
                 * @param result the output parameter containing the the result
                 *               probability and possibly some additional meta
                 *               data for the decoder.
                 */
                void query(const T_M_Gram & ngram, TQueryResult & result) {
                    const TModelLevel level = ngram.level;

                    //Check the number of elements in the N-Gram
                    if (DO_SANITY_CHECKS && ((level < M_GRAM_LEVEL_1) || (level > N))) {
                        stringstream msg;
                        msg << "An improper N-Gram size, got " << level << ", must be between [1, " << N << "]!";
                        throw Exception(msg.str());
                    } else {
                        //First transform the given M-gram into word hashes.
                        ATrie<N>::store_m_gram_word_ids(ngram);

                        //Go on with a recursive procedure of computing the N-Gram probabilities
                        get_probability(level, result.prob);

                        LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE << " probability is: " << result.prob << END_LOG;
                    }
                }

                /**
                 * Allows to retrieve the stored word index, if any
                 * @return the pointer to the stored word index or NULL if none
                 */
                inline AWordIndex * get_word_index() {
                    return m_p_word_index;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~ATrie() {
                    if (m_p_word_index != NULL) {
                        delete m_p_word_index;
                    }
                };

            protected:
                //Stores the reference to the word index to be used
                AWordIndex * m_p_word_index;

                //The temporary data structure to store the N-gram word ids
                TShortId m_gram_word_ids[N];

                /**
                 * Gets the word hash for the end word of the back-off N-Gram
                 * @return the word hash for the end word of the back-off N-Gram
                 */
                inline const TShortId & get_back_off_end_word_id() {
                    return m_gram_word_ids[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the N-gram
                 * @return the word hash for the last word in the N-gram
                 */
                inline const TShortId & get_end_word_id() {
                    return m_gram_word_ids[N - 1];
                }

                /**
                 * Converts the given tokens to ids and stores it in
                 * m_gram_word_ids. The ids are aligned to the backof the
                 * array so that the last m_gram_word_ids[N-1] always 
                 * stores the id of the last word.
                 * @param ngram the n-gram tokens to convert to hashes
                 */
                inline void store_m_gram_word_ids(const T_M_Gram & ngram) {
                    if (DO_SANITY_CHECKS && (m_p_word_index == NULL)) {
                        throw Exception("The m_p_word_index is not set!");
                    }

                    //Computethe begin index in m_gram_word_ids to start putting ids from
                    const TModelLevel begin_idx = N - ngram.level;

                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - ngram.level;
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(ngram.level) << "-gram:" << END_LOG;
                    for (TModelLevel i = 0; i < ngram.level; i++) {
                        //Do not check whether the word was found or not, if it was not then the id is UNKNOWN_WORD_ID
                        m_p_word_index->get_word_id(ngram.tokens[i].str(), m_gram_word_ids[begin_idx + idx]);
                        LOG_DEBUG1 << "wordId('" << ngram.tokens[i].str() << "') = " << SSTR(m_gram_word_ids[begin_idx + idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * This method will be called after all the 1-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_1_grams() {
                };

                /**
                 * This method will be called after all the M-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @param level the level of the M-grams that were finished to be read
                 */
                virtual void post_m_grams(const TModelLevel level) {
                };

                /**
                 * This method will be called after all the N-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_n_grams() {
                };

                /**
                 * This function should be called in case we can not get the probability for
                 * the given M-gram and we want to compute it's back-off probability instead
                 * @param ctxLen the length of the context for the M-gram for which we could
                 * not get the probability from the trie.
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                inline void get_back_off_prob(const TModelLevel ctxLen, TLogProbBackOff & prob) {
                    //Compute the lover level probability
                    get_probability(ctxLen, prob);

                    LOG_DEBUG1 << "getProbability(" << ctxLen
                            << ") = " << prob << END_LOG;

                    //If the probability is not zero then go on with computing the
                    //back-off. Otherwise it does not make sence to compute back-off.
                    if (prob > ZERO_LOG_PROB_WEIGHT) {
                        TLogProbBackOff back_off;
                        if (!get_back_off_weight(ctxLen, back_off)) {
                            //Set the back-off weight value to zero as there is no back-off found!
                            back_off = ZERO_BACK_OFF_WEIGHT;
                        }

                        LOG_DEBUG1 << "getBackOffWeight(" << ctxLen
                                << ") = " << back_off << END_LOG;

                        LOG_DEBUG2 << "The " << ctxLen << " probability = " << back_off
                                << " + " << prob << " = " << (back_off + prob) << END_LOG;

                        //Do the back-off weight plus the lower level probability, we do a plus as we work with LOG probabilities
                        prob += back_off;
                    }
                }

                /**
                 * This function should implement the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes can be obtained from the _wordHashes member
                 * variable of the class.
                 * @param level the M-gram level for which the probability is to be computed
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                virtual void get_probability(const TModelLevel level, TLogProbBackOff & prob) = 0;

                /**
                 * This function allows to get the back-off weight for the current context.
                 * The N-Gram hashes can be obtained from the pre-computed data member array
                 * _wordHashes.
                 * @param level the M-gram level for which the back-off weight is to be found,
                 * is equal to the context length of the K-Gram in the caller function
                 * @param back_off [out] the back-off weight to be computed
                 * @return the resulting back-off weight probability
                 */
                virtual bool get_back_off_weight(const TModelLevel level, TLogProbBackOff & back_off) = 0;

            };
        }
    }
}

#endif	/* ATRIE_HPP */

