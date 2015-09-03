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
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HashingUtils.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

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
             * This structure is used to store the N-Gram data
             * of the back-off Language Model.
             * @param prob stores the log_10 probability of the N-Gram Must be
             *             a negative value
             * @param back_off stores the log_10 back-off weight (probability)
             *        of the N-gram can be 0 is the probability is not available
             * @param context stores the n-gram's context i.e. for "w1 w2 w3" -> "w1 w2"
             * @param tokens stores the N-gram words the size of this vector
             *        defines the N-gram level.
             * @param level stores the number of meaningful elements in the tokens, the value of N for the N-gram
             */
            typedef struct {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
                TextPieceReader context;
                TextPieceReader tokens[MAX_NGRAM_LEVEL];
                TModelLevel level;

                /**
                 * This function allows to compute the hash of the given M-Gram
                 * It assumes, which should hold, that the memory pointed by the tokens is continuous
                 * @return the hash value of the given token
                 */
                template<size_t number_buckets>
                inline TShortId hash() {
                    //Compute the length of the gram tokens in memory, including spaces between
                    const char * beginFirtPtr = tokens[0].getBeginCStr();
                    const TextPieceReader & last = tokens[level - 1];
                    const char * beginLastPtr = last.getBeginCStr();
                    const size_t totalLen = (beginLastPtr - beginFirtPtr) + last.getLen();

                    //If the sanity check is on then test that the memory is continuous
                    //Compute the same length but with a longer iterative algorithms
                    if (DO_SANITY_CHECKS) {
                        //Compute the exact length
                        size_t exactTotalLen = level - 1 ; //The number of spaces in between tokens
                        for (TModelLevel idx = 0; idx < level; idx++) {
                            exactTotalLen += tokens[idx].getLen();
                        }
                        //Check that the exact and fast computed lengths are the same
                        if (exactTotalLen != totalLen) {
                            stringstream msg;
                            msg << "The memory allocation for M-gram tokens is not continuous: totalLen ("  <<
                                    SSTR(totalLen) << ") != exactTotalLen (" << SSTR(exactTotalLen) << ")";
                            throw Exception(msg.str());
                        }
                    }

                    //Compute the hash using the gram tokens with spaces with them
                    return computePaulHsiehHash(beginFirtPtr, totalLen) % number_buckets;
                }
            } T_M_Gram;

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

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit ATrie(AWordIndex * const _pWordIndex)
                : m_p_word_index(_pWordIndex) {
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void preAllocate(const size_t counts[N]) {
                    m_p_word_index->reserve(counts[0]);
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param oGram the 1-Gram data
                 */
                virtual void add_1_Gram(const T_M_Gram &oGram) = 0;

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param mGram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void add_M_Gram(const T_M_Gram &mGram) = 0;

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param nGram the N-Gram data
                 */
                virtual void add_N_Gram(const T_M_Gram &nGram) = 0;

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * @param level the level of the X-grams that were finished to be read
                 */
                virtual bool isPost_Grams(const TModelLevel level) {
                    return false;
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * @param level the level of the X-grams that were finished to be read
                 */
                void post_Grams(const TModelLevel level) {
                    switch (level) {
                        case ONE_GRAM_LEVEL:
                            this->post_1_Grams();
                            break;
                        case N:
                            this->post_N_Grams();
                            break;
                        default:
                            this->post_M_Grams(level);
                            break;
                    }
                };

                /**
                 * Returns the maximum length of the considered N-Grams
                 * @return the maximum length of the considered N-Grams
                 */
                TModelLevel getNGramLevel() const {
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
                virtual void queryNGram(const T_M_Gram & ngram, TQueryResult & result) = 0;

                /**
                 * Allows to retrieve the stored word index, if any
                 * @return the pointer to the stored word index or NULL if none
                 */
                inline AWordIndex * getWordIndex() {
                    return m_p_word_index;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~ATrie() {
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
                inline void tokensToId(const T_M_Gram & ngram, TShortId wordHashes[N]) {
                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - ngram.level;
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(ngram.level) << "-gram:" << END_LOG;
                    for (TModelLevel i = 0; i < ngram.level; i++) {
                        //Do not check whether the word was found or not, if it was not then the id is UNKNOWN_WORD_ID
                        m_p_word_index->getId(ngram.tokens[i].str(), wordHashes[idx]);
                        LOG_DEBUG1 << "wordId('" << ngram.tokens[i].str() << "') = " << SSTR(wordHashes[idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * This method will be called after all the 1-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_1_Grams() {
                };

                /**
                 * This method will be called after all the M-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @param level the level of the M-grams that were finished to be read
                 */
                virtual void post_M_Grams(const TModelLevel level) {
                };

                /**
                 * This method will be called after all the N-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_N_Grams() {
                };

            private:
                //Stores the reference to the word index to be used
                AWordIndex * const m_p_word_index;

            };
        }
    }
}

#endif	/* ATRIE_HPP */

