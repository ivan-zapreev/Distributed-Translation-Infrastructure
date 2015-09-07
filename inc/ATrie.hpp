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
                virtual void query(const T_M_Gram & ngram, TQueryResult & result) = 0;

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

                /**
                 * This method converts the M-Gram tokens into hashes and stores
                 * them in an array. Note that, M is the size of the tokens array.
                 * It is not checked, for the sake of performance but is assumed
                 * that M is <= N!
                 * @param ngram the n-gram structure with the query ngram tokens
                 * @param wordHashes the out array parameter to store the hashes.
                 */
                inline void tokens_to_id(const T_M_Gram & ngram, TShortId wordHashes[N]) {
                    if (m_p_word_index != NULL) {
                        //The start index depends on the value M of the given M-Gram
                        TModelLevel idx = N - ngram.level;
                        LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(ngram.level) << "-gram:" << END_LOG;
                        for (TModelLevel i = 0; i < ngram.level; i++) {
                            //Do not check whether the word was found or not, if it was not then the id is UNKNOWN_WORD_ID
                            m_p_word_index->get_word_id(ngram.tokens[i].str(), wordHashes[idx]);
                            LOG_DEBUG1 << "wordId('" << ngram.tokens[i].str() << "') = " << SSTR(wordHashes[idx]) << END_LOG;
                            idx++;
                        }
                    } else {
                        throw Exception("The m_p_word_index is not set!");
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

            private:
                //Stores the reference to the word index to be used
                AWordIndex * m_p_word_index;

            };
        }
    }
}

#endif	/* ATRIE_HPP */

