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

            //This typedef defines a frequency result type, which is a reference to
            //an array of N frequency values. Note that the value with index [0] will
            //contain the frequency for the 1-gram en so forth.

            template<TModelLevel N> struct SFrequencyResult {
                TFrequencySize result[N];
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
                 * This method adds a words to the trie
                 * @param tokens the array of tokens/words to add the trie from
                 */
                virtual void addWords(vector<string> &tokens) = 0;

                /**
                 * This method adds a new n-gram into the trie
                 * @param tokens the array of tokens to add the trie from
                 * @param idx the index to start with
                 * @param n the value of "n" for the n-gram (the number of elements in the n-gram).
                 */
                virtual void addNGram(vector<string> &tokens, const int idx, const int n) = 0;

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
                 * This function will query the trie for all the n-grams finishing with the given word.
                 * This function is to be used when no query result caching is done.
                 * @param word the word that is begin queued
                 * @param wrap the reference of an array into which the result values will 
                 *               be put. The result[0] will contain frequency for 0-gram etc.
                 * @throws Exception in case this function is used when query result caching is done.
                 */
                virtual void queryWordFreqs(const string & word, SFrequencyResult<N> & result) throw (Exception) = 0;

                /**
                 * This function will query the trie for all the n-grams finishing with the given word.
                 * This function is to be used when result caching is done.
                 * @param word the word that is begin queued
                 * @return the reference to a cached array into which the result values will
                 *         be put. The result[0] will contain frequency for 1-gram etc.
                 * @throws Exception in case this function is used when query result caching is not done.
                 */
                virtual SFrequencyResult<N> & queryWordFreqs(const string & word) throw (Exception) = 0;

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the frequencies of the sub n-grams computed as:
                 * freqs[0] = frequency( [word1 word2 word3 word4 word5] )
                 * freqs[1] = frequency( [word2 word3 word4 word5] )
                 * freqs[2] = frequency( [word3 word4 word5] )
                 * freqs[3] = frequency( [word4 word5] )
                 * freqs[4] = frequency( [word5] )
                 * @param ngram the given N-gram vector is expected to have exactly N elements (see the template parameters)
                 * @param freqs the array into which the frequencies will be placed.
                 */
                virtual void queryNGramFreqs(const vector<string> & ngram, SFrequencyResult<N> & freqs) = 0;

                /**
                 * Allows to force reset of internal query caches, if they exist
                 */
                virtual void resetQueryCache() = 0;
            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<N_GRAM_PARAM, true> TFiveCacheTrie;
            typedef ATrie<N_GRAM_PARAM, false> TFiveNoCacheTrie;

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ATrie<N_GRAM_PARAM, true>;
            template class ATrie<N_GRAM_PARAM, false>;
        }
    }
}
#endif	/* ITRIES_HPP */

