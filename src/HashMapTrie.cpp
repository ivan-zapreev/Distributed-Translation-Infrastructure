/* 
 * File:   HashMapTrie.cpp
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
#include "HashMapTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm>      //std::fill

#include "Logger.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, bool doCache>
            const TModelLevel HashMapTrie<N, doCache>::MINIMUM_CONTEXT_LEVEL = 2;

            template<TModelLevel N, bool doCache>
            HashMapTrie<N, doCache>::HashMapTrie() {
            }

            template<TModelLevel N, bool doCache>
            string HashMapTrie<N, doCache>::ngramToString(const vector<string> &tokens) {
                string str = "[ ";
                for (vector<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                    str += *it + " ";
                }
                return str + "]";
            }

            template<TModelLevel N, bool doCache>
            void HashMapTrie<N, doCache>::preAllocate(uint counts[N]) {
                //ToDo: Implement this method once we know what are the
                //approximate % values for the number of N-Grams ending
                //with the same word for each specific N.
                LOG_WARNING << "The Trie memory is not allocated efficiently yet! Try using the N-Gram Counts!" << END_LOG;
            }

            template<TModelLevel N, bool doCache>
            void HashMapTrie<N, doCache>::add1Gram(const SBackOffNGram &oGram) {
                //First get the token/word from the 1-Gram
                const string & token = oGram.tokens[0];

                LOG_DEBUG << "Adding a 1-Gram: '" << token << "' to the Trie." << END_LOG;

                //Compute it's hash value
                TWordHashSize wordHash = computeHash(token);
                //Get the word data storing structure from the list of words
                TWordEntryPair & wordData = oGrams[wordHash];
                //Get the word probability and back-off data reference
                TProbBackOffEntryPair & pbData = wordData.second;

                //If the probability is zero then this word has not been seen yet
                if (pbData.first == UNDEFINED_LOG_PROB_WEIGHT) {
                    wordData.first = token;
                } else {
                    //If the word probability is set then it is either
                    if (wordData.first.compare(token)) {
                        stringstream msg;
                        msg << "Hash collision: '" << token << "' and '" << wordData.first << "' both have hash: " << wordHash;
                        throw Exception(msg.str());
                    } else {
                        //The word has been seen already, this is a potential error, so we report a warning!
                        LOG_WARNING << "The 1-Gram/Word: '" << token << "' has been already seen! "
                                << "Changing the (prob,back-off) data from ("
                                << pbData.first << "," << pbData.second << ") to ("
                                << oGram.prob << "," << oGram.back_off << ")" << END_LOG;
                    }
                }
                //Set/Update the probability and back-off values for the word
                pbData.first = oGram.prob;
                pbData.second = oGram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.first << "," << pbData.second << ") for "
                        << ngramToString(oGram.tokens) << " wordHash = "
                        << wordHash << END_LOG;
            }

            template<TModelLevel N, bool doCache>
            void HashMapTrie<N, doCache>::addMGram(const SBackOffNGram &mGram) {
                const size_t level = mGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << ngramToString(mGram.tokens) << " to the Trie" << END_LOG;

                //Check that this is not an 1-Gram or N-Gram for those we need another method!
                if ((MIN_NGRAM_LEVEL < level) || (level < N)) {
                    //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                    // 1. Compute the context hash defined by w1 w2 w3
                    TReferenceHashSize contextHash = computeHashContext(mGram.tokens);

                    // 2. Compute the hash of w4
                    const string & endWord = *(--mGram.tokens.end());
                    TWordHashSize wordHash = computeHash(endWord);
                    LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                    // 3. Insert the probability data into the trie
                    //Data stores the N-tires from length 2 on, therefore "idx-1"
                    //Get/Create the mapping for this word in the Trie level of the N-gram
                    TMGramEntryMap& ngamEntry = mGrams[level - 1][wordHash];
                    //Get/Create the new Prob. and Back-Off entry pair in the map, for the context
                    TProbBackOffEntryPair& pbData = ngamEntry[contextHash];

                    //Check that the probability data is not set yet, otherwise a warning!
                    if (pbData.first != UNDEFINED_LOG_PROB_WEIGHT) {
                        //The M-Gram has been seen already, this is a potential error, so we report a warning!
                        LOG_WARNING << "The " << level << "-Gram : '" << ngramToString(mGram.tokens) << "' has been already seen! "
                                << "Changing the (prob,back-off) data from ("
                                << pbData.first << "," << pbData.second << ") to ("
                                << mGram.prob << "," << mGram.back_off << ")" << END_LOG;
                    }

                    //Set/Update the probability and back-off values for the word
                    pbData.first = mGram.prob;
                    pbData.second = mGram.back_off;

                    LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                            << pbData.first << "," << pbData.second << ") for "
                            << ngramToString(mGram.tokens) << " contextHash = "
                            << contextHash << ", wordHash = " << wordHash << END_LOG;
                } else {
                    stringstream msg;
                    msg << "Internal error: The " << level << "-Grams are to be handled with another add method!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N, bool doCache>
            void HashMapTrie<N, doCache>::addNGram(const SBackOffNGram &nGram) {
                const size_t level = nGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << ngramToString(nGram.tokens) << " to the Trie" << END_LOG;

                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TReferenceHashSize contextHash = computeHashContext(nGram.tokens);

                // 2. Compute the hash of w4
                const string & endWord = *(--nGram.tokens.end());
                TWordHashSize wordHash = computeHash(endWord);
                LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                // 3. Insert the probability data into the trie
                //Data stores the N-tires from length 2 on, therefore "idx-1"
                //Get/Create the mapping for this word in the Trie level of the N-gram
                TNGramEntryMap& ngamEntry = nGrams[wordHash];
                //Get/Create the new Prob. in the map, for the context
                TLogProbBackOff& pData = ngamEntry[contextHash];

                //Check that the probability data is not set yet, otherwise a warning!
                if (pData != UNDEFINED_LOG_PROB_WEIGHT) {
                    //The M-Gram has been seen already, this is a potential error, so we report a warning!
                    LOG_WARNING << "The " << level << "-Gram : '" << ngramToString(nGram.tokens)
                            << "' has been already seen! "
                            << "Changing the prob. data from ("
                            << pData << ") to (" << nGram.prob << ")" << END_LOG;
                }

                //Set/Update the probability
                pData = nGram.prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << pData << ") for "
                        << ngramToString(nGram.tokens) << " contextHash = "
                        << contextHash << ", wordHash = " << wordHash << END_LOG;
            }

            template<TModelLevel N, bool doCache>
            void HashMapTrie<N, doCache>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                //ToDo: implement!
            }

            template<TModelLevel N, bool doCache>
            HashMapTrie<N, doCache>::HashMapTrie(const HashMapTrie& orig) {
            }

            template<TModelLevel N, bool doCache>
            HashMapTrie<N, doCache>::~HashMapTrie() {
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class HashMapTrie<MAX_NGRAM_LEVEL, true>;
            template class HashMapTrie<MAX_NGRAM_LEVEL, false>;
        }
    }
}