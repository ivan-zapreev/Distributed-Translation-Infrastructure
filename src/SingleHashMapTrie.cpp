/* 
 * File:   SingleHashMapTrie.cpp
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
 * Created on April 18, 2015, 11:42 AM
 */
#include "SingleHashMapTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "Logger.hpp"
#include "StringUtils.hpp"

using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            SingleHashMapTrie<N>::SingleHashMapTrie() : AHashMapTrie<N>() {
                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntryPair & pbData = ngRecorder[0][UNKNOWN_WORD_HASH][UNDEFINED_WORD_HASH];
                pbData.first = MINIMAL_LOG_PROB_WEIGHT;
                pbData.second = UNDEFINED_LOG_PROB_WEIGHT;
            }

            template<TModelLevel N>
            void SingleHashMapTrie<N>::preAllocate(uint counts[N]) {
                //ToDo: Implement this method once we know what are the
                //approximate % values for the number of N-Grams ending
                //with the same word for each specific N.
                LOG_WARNING << "The Trie memory is not allocated efficiently yet! Try using the N-Gram Counts!" << END_LOG;
            }

            template<TModelLevel N>
            void SingleHashMapTrie<N>::add1Gram(const SBackOffNGram &oGram) {
                addGram<true>(oGram);
            }

            template<TModelLevel N>
            void SingleHashMapTrie<N>::addMGram(const SBackOffNGram &mGram) {
                addGram<false>(mGram);
            }

            template<TModelLevel N>
            void SingleHashMapTrie<N>::addNGram(const SBackOffNGram &nGram) {
                addGram<false>(nGram);
            }

            template<TModelLevel N>
            TLogProbBackOff SingleHashMapTrie<N>::getBackOffWeight(const TModelLevel contextLength) {
                //Get the word hash for the en word of the back-off N-Gram
                const TWordHashSize & endWordHash = AHashMapTrie<N>::getBackOffNGramEndWordHash();

                //Compute the back-off context length
                const TModelLevel backOfContextLength = contextLength - 1;

                LOG_DEBUG1 << "Computing back-off for an " << (backOfContextLength + 1)
                        << "-gram the context length is " << backOfContextLength << END_LOG;

                //Compute the context hash variable
                const TReferenceHashSize contextHash = (backOfContextLength > 0) ? AHashMapTrie<N>::computeHashContext(backOfContextLength, true) : UNDEFINED_WORD_HASH;

                //Set the initial back-off weight value to undefined!
                TLogProbBackOff back_off = ZERO_LOG_PROB_WEIGHT;

                //Attempt to retrieve back-off weights
                try {
                    //Get the back-off N-gram level index
                    const TModelLevel levelIdx = (backOfContextLength+1)-1;
                    TProbBackOffEntryPair & entry = ngRecorder[levelIdx].at(endWordHash).at(contextHash);

                    //Obtained the stored back-off weight
                    back_off = entry.second;

                    LOG_DEBUG2 << "The " << contextLength << "-Gram log_"
                            << LOG_PROB_WEIGHT_BASE << "( back-off ) for (word, context)=("
                            << endWordHash << ", " << contextHash << "), is: " << back_off << END_LOG;
                } catch (out_of_range e) {
                    LOG_DEBUG << "Unable to find the " << (contextLength)
                            << "-Gram entry for a (word, context)=("
                            << endWordHash << ", " << contextHash << "), need to back off!" << END_LOG;
                }

                LOG_DEBUG2 << "The chosen log back-off weight for context: " << contextLength << " is: " << back_off << END_LOG;

                //Return the computed back-off weight it can be UNDEFINED_LOG_PROB_WEIGHT, which is zero - no penalty
                return back_off;
            }

            template<TModelLevel N>
            TLogProbBackOff SingleHashMapTrie<N>::computeLogProbability(const TModelLevel contextLength) {
                //Define the result variable to store the probability and set it to "zero"
                TLogProbBackOff prob = MINIMAL_LOG_PROB_WEIGHT;

                LOG_DEBUG1 << "Computing probability for an " << (contextLength + 1)
                        << "-gram the context length is " << contextLength << END_LOG;

                //Get the last word in the N-gram
                const TWordHashSize & endWordHash = AHashMapTrie<N>::getNGramEndWordHash();

                //Define the context hash and initialize it
                const TReferenceHashSize contextHash = (contextLength > 0) ? AHashMapTrie<N>::computeHashContext(contextLength, false) : UNDEFINED_WORD_HASH;

                //Attempt to retrieve probabilities
                try {
                    //Get the N-gram level index
                    const TModelLevel levelIdx = (contextLength+1)-1;
                    TProbBackOffEntryPair & entry = ngRecorder[levelIdx].at(endWordHash).at(contextHash);

                    LOG_DEBUG2 << "The " << (contextLength + 1)
                            << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                            << "( prob. ) for (word,context) = ("
                            << endWordHash << ", " << contextHash
                            << "), is: " << entry.first << END_LOG;

                    //Return the stored probability
                    prob = entry.first;
                } catch (out_of_range e) {
                    LOG_DEBUG << "Unable to find the " << (contextLength + 1)
                            << "-Gram  prob for a (word,context) = ("
                            << endWordHash << ", " << contextHash
                            << "), need to back off!" << END_LOG;

                    //If we could not find the probability then try backing off, if possible, otherwise return "zero"
                    if (contextLength > 0) {
                        const TLogProbBackOff back_off = getBackOffWeight(contextLength);
                        const TLogProbBackOff subProb = computeLogProbability(contextLength - 1);

                        LOG_DEBUG1 << "getBackOffWeight(" << contextLength << ") = " << back_off
                                << ", computeLogProbability(" << (contextLength - 1) << ") = "
                                << subProb << END_LOG;

                        LOG_DEBUG2 << "The " << contextLength << " probability = " << back_off
                                << " + " << subProb << " = " << (back_off + subProb) << END_LOG;

                        //Do the back-off weight plus the lower level probability, we do a plus as we work with LOG probabilities
                        prob = back_off + subProb;
                    }
                }

                LOG_DEBUG2 << "The chosen log probability for context: " << contextLength << " is: " << prob << END_LOG;

                return prob;
            }

            template<TModelLevel N>
            void SingleHashMapTrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                const TModelLevel mGramLength = ngram.size();
                //Check the number of elements in the N-Gram
                if ((1 <= mGramLength) && (mGramLength <= N)) {
                    //First transform the given M-gram into word hashes.
                    AHashMapTrie<N>::storeNGramHashes(ngram);

                    //Go on with a recursive procedure of computing the N-Gram probabilities
                    result.prob = computeLogProbability(mGramLength - 1);

                    LOG_DEBUG << "The computed log_" << LOG_PROB_WEIGHT_BASE << " probability is: " << result.prob << END_LOG;
                } else {
                    stringstream msg;
                    msg << "An improper N-Gram size, got " << mGramLength << ", must be between [1, " << N << "]!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N>
            SingleHashMapTrie<N>::SingleHashMapTrie(const SingleHashMapTrie& orig) {
            }

            template<TModelLevel N>
            SingleHashMapTrie<N>::~SingleHashMapTrie() {
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class SingleHashMapTrie<MAX_NGRAM_LEVEL>;
        }
    }
}