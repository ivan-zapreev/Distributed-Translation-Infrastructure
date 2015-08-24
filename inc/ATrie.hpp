/* 
 * File:   ATries.hpp
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
 * Created on April 18, 2015, 11:38 AM
 */

#ifndef ITRIES_HPP
#define	ITRIES_HPP

#include <vector> //std::vector
#include <string> //std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"

using namespace std;
using namespace uva::smt::exceptions;
using namespace uva::smt::hashing;
using namespace uva::smt::tries;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            //This macro is needed to report the collision detection warnings!
#define REPORT_COLLISION_WARNING(N, gram, wordHash, contextId, prevProb, prevBackOff, newProb, newBackOff)   \
            LOG_WARNING << "The " << gram.level << "-Gram : " << tokensToString<N>(gram.tokens, gram.level)  \
                        << " has been already seen! Word Id: " << SSTR(wordHash)                             \
                        << ", context Id: " << SSTR(contextId) << ". "                                       \
                        << "Changing the (prob,back-off) data from ("                                        \
                        << prevProb << "," << prevBackOff << ") to ("                                        \
                        << newProb << "," << newBackOff << ")" << END_LOG;

            //Stores the Bi-Gram level value
            const TModelLevel BGRAM_LEVEL_VALUE = 2;

            //The type used for storing log probabilities and back-off values
            typedef float TLogProbBackOff;

            //The base of the logarithm for stored probabilities/back-off weights
            const TLogProbBackOff LOG_PROB_WEIGHT_BASE = 10.0;
            //The zero value for probability/back-off weight
            const TLogProbBackOff ZERO_LOG_PROB_WEIGHT = 0.0f;
            //The value indicating an undefined probability/back-off weight
            const TLogProbBackOff UNDEFINED_LOG_PROB_WEIGHT = 99.0f;
            //The value of the minimal probability/back-off weight
            const TLogProbBackOff MINIMAL_LOG_PROB_WEIGHT = -10.0f;
            //The zerro like value for probability/back-off weight
            const TLogProbBackOff ZERRO_LOG_PROB_WEIGHT = -100.0f;

            /**
             * This structure is used to define the trivial probability/
             * back-off pari to be stored for M-grams with 1 <= M < N
             * @param prob stores the probability
             * @param back_off stores the back-off
             */
            typedef struct {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
            } TProbBackOffEntryPair;

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
            struct SRawNGram {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
                TextPieceReader context;
                TextPieceReader tokens[MAX_NGRAM_LEVEL];
                TModelLevel level;
            };

            /**
             * This structure is used to store the N-Gram data
             * of the back-off Language Model.
             * @param prob stores the log_10 probability of the N-Gram Must be
             *             a negative value
             * @param back_off stores the log_10 back-off weight (probability)
             *        of the N-gram can be 0 is the probability is not available
             * @param tokens stores the N-gram words the size of this vector
             *        defines the N-gram level.
             */
            struct SNiceNGram {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
                vector<string> tokens;
            };

            /**
             * This data structure is to be used to return the N-Gram query result.
             * It contains the computed Back-Off language model probability and
             * potentially additional meta data for the decoder
             * @param prob the computed Back-Off language model probability as log_${LOG_PROB_WEIGHT_BASE}
             */
            struct SProbResult {
                float prob;
            };

            /**
             * This is a common abstract class for all possible Trie implementations
             * The purpose of having this as a template class is performance optimization.
             * It is a template class that has two template parameters:
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             * @param doCache - the indicative flag that asks the child class to, if possible,
             *                  cache the queries.
             */
            template<TModelLevel N>
            class ATrie {
            public:

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit ATrie(AWordIndex * const _pWordIndex)
                : pWordIndex(_pWordIndex), chachedLevel(UNDEF_NGRAM_LEVEL),
                chachedContext(contextCStr, MAX_N_GRAM_STRING_LENGTH),
                chachedContextId(UNDEFINED_WORD_ID) {
                };

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void preAllocate(const size_t counts[N]) = 0;

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param oGram the 1-Gram data
                 */
                virtual void add1Gram(const SRawNGram &oGram) = 0;

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param mGram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void addMGram(const SRawNGram &mGram) = 0;

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param nGram the N-Gram data
                 */
                virtual void addNGram(const SRawNGram &nGram) = 0;

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
                 * @param ngram the given N-gram vector is expected to have
                 *              exactly N elements (see the template parameters)
                 * @param result the output parameter containing the the result
                 *               probability and possibly some additional meta
                 *               data for the decoder.
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result) = 0;

                /**
                 * Allows to retrieve the stored word index, if any
                 * @return the pointer to the stored word index or NULL if none
                 */
                inline AWordIndex * getWordIndex() {
                    return pWordIndex;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~ATrie() {
                };
            protected:

                /**
                 * Allows to retrieve the cached context id for the given M-gram if any
                 * @param mGram the m-gram to get the context id for
                 * @param result the output parameter, will store the cached id, if any
                 * @return true if there was nothing cached, otherwise false
                 */
                inline bool getCachedContextId(const SRawNGram &mGram, TContextId & result) {
                    if (chachedLevel == mGram.level) {
                        if (chachedContext == mGram.context) {
                            result = chachedContextId;
                            LOG_INFO3 << "Cache match! " << chachedContext << " == " << mGram.context << "!" << END_LOG;
                            return false;
                        }
                    }
                    return true;
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void cacheContextId(const SRawNGram &mGram, TContextId & stx_id) {
                    chachedLevel = mGram.level;
                    chachedContext.copy_string<MAX_N_GRAM_STRING_LENGTH>(mGram.context);
                    chachedContextId = stx_id;
                }

            private:
                //Stores the reference to the word index to be used
                AWordIndex * const pWordIndex;

                //Stores the cached M-gram level M (for 1 < M <= N )
                TModelLevel chachedLevel;
                //The actual storage for the cached context c string
                char contextCStr[MAX_N_GRAM_STRING_LENGTH];
                //Stores the cached M-gram context (for 1 < M <= N )
                TextPieceReader chachedContext;
                //Stores the cached M-gram context value (for 1 < M <= N )
                TContextId chachedContextId;
            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<MAX_NGRAM_LEVEL> TFiveTrie;

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ATrie<MAX_NGRAM_LEVEL>;
        }
    }
}
#endif	/* ITRIES_HPP */

