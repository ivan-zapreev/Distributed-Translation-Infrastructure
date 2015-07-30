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

using namespace std;
using namespace uva::smt::exceptions;

namespace uva {
    namespace smt {
        namespace tries {

            //The base of the logarithm for stored probabilities/back-off weights
#define LOG_PROB_WEIGHT_BASE 10.0d
            //The zero value for probability/back-off weight
#define ZERO_LOG_PROB_WEIGHT 0.0f
            //The value indicating an undefined probability/back-off weight
#define UNDEFINED_LOG_PROB_WEIGHT 99.0f
            //The value of the minimal probability/back-off weight
#define MINIMAL_LOG_PROB_WEIGHT -10.0f
            //The zerro like value for probability/back-off weight
#define ZERRO_LOG_PROB_WEIGHT -99.0f

            //The type used for storing log probabilities and back-off values
            typedef float TLogProbBackOff;

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
            struct SBackOffNGram {
                TLogProbBackOff prob;
                TLogProbBackOff back_off;
                vector<string> tokens;
            };

            /**
             * This data structure is to be used to return the N-Gram query result.
             * It contains the computed Back-Off language model probability and
             * potentially additional meta data for the decoder
             * @param prob the computed Back-Off language model probability
             */
            struct SProbResult {
                double prob;
            };

            /**
             * This is a common abstract class for all possible Trie implementations
             * The purpose of having this as a template class is performance optimization.
             * It is a template class that has two template parameters:
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             * @param doCache - the indicative flag that asks the child class to, if possible,
             *                  cache the queries.
             */
            template<TModelLevel N, bool doCache>
            class ATrie {
            public:

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void preAllocate(uint counts[N]) = 0;

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param oGram the 1-Gram data
                 */
                virtual void add1Gram(const SBackOffNGram &oGram) = 0;

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param mGram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void addMGram(const SBackOffNGram &mGram) = 0;

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param nGram the N-Gram data
                 */
                virtual void addNGram(const SBackOffNGram &nGram) = 0;

                /**
                 * Returns the maximum length of the considered N-Grams
                 * @return the maximum length of the considered N-Grams
                 */
                TModelLevel getNGramLevel() const {
                    return N;
                }

                /**
                 * Allows to test if the given Trie implementation has internal query caches
                 * @return true if the cache exists otherwise false
                 */
                virtual bool doesQueryCache() const {
                    return doCache;
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
                 * Allows to force reset of internal query caches, if they exist
                 */
                virtual void resetQueryCache() = 0;
            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<MAX_NGRAM_LEVEL, true> TFiveCacheTrie;
            typedef ATrie<MAX_NGRAM_LEVEL, false> TFiveNoCacheTrie;

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ATrie<MAX_NGRAM_LEVEL, true>;
            template class ATrie<MAX_NGRAM_LEVEL, false>;
        }
    }
}
#endif	/* ITRIES_HPP */

