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
#include "MultiHashMapTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "Logger.hpp"
#include "StringUtils.hpp"

using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            //The following is to be used for additional monitoring of collisions
#define MONITORE_COLLISIONS
#ifdef MONITORE_COLLISIONS

            template<TModelLevel N>
            void MultiHashMapTrie<N>::recordAndCheck(const TWordHashSize wordHash,
                    const TReferenceHashSize contextHash, const SBackOffNGram &gram) {
                //First try to get the entries for the given word
                try {
                    unordered_map<TReferenceHashSize, vector < string>> &entries = ngRecorder.at(wordHash);
                    //Second try to get the entries for the given context
                    try {
                        vector<string> entry = entries.at(contextHash);

                        //If we could get the values, then it is important to check
                        //that the length is the same. If it is then we have context
                        //hash collisions for the same length N-grams and this must
                        //not be happening! Then we will print a lot of debug info!
                        const TModelLevel size = entry.size();
                        if (size == gram.tokens.size()) {
                            LOG_WARNING << "N-gram collision/duplicates: '" << ngramToString(entry) << "' with '"
                                    << ngramToString(gram.tokens) << "'! wordHash= " << wordHash
                                    << ", contextHash= " << contextHash << END_LOG;

                            TWordHashSize old[N], fresh[N];
                            AHashMapTrie<N>::tokensToHashes(entry, old);
                            AHashMapTrie<N>::tokensToHashes(gram.tokens, fresh);
                            for (int i = 0; i < size; i++) {
                                LOG_INFO << i << ") wordHash(" << entry[i] << ") = " << old[N - size + i] << END_LOG;
                                LOG_INFO << i << ") wordHash(" << gram.tokens[i] << ") = " << fresh[N - size + i] << END_LOG;
                            }
                            AHashMapTrie<N>::template computeHashContext<Logger::INFO>(entry);
                            AHashMapTrie<N>::template computeHashContext<Logger::INFO>(gram.tokens);
                        }
                    } catch (out_of_range e) {
                        //If no entries for the context, add a new one
                        entries[contextHash] = gram.tokens;
                    }
                } catch (out_of_range e) {
                    //If no entries for the given word and thus context add a new one
                    ngRecorder[wordHash][contextHash] = gram.tokens;
                }
            }
#else

            template<TModelLevel N>
            void MultiHashMapTrie<N>::recordAndCheck(const TWordHashSize wordHash,
                    const TReferenceHashSize contextHash, const SBackOffNGram &gram) {
            }
#endif

            template<TModelLevel N>
            MultiHashMapTrie<N>::MultiHashMapTrie() : AHashMapTrie<N>() {
                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntryPair & pbData = oGrams[UNKNOWN_WORD_HASH];
                pbData.first = MINIMAL_LOG_PROB_WEIGHT;
                pbData.second = UNDEFINED_LOG_PROB_WEIGHT;

                LOG_INFO << "Using the " << __FILE__ << " model!" << END_LOG;
            }

            template<TModelLevel N>
            void MultiHashMapTrie<N>::preAllocate(uint counts[N]) {
                //ToDo: Implement this method once we know what are the
                //approximate % values for the number of N-Grams ending
                //with the same word for each specific N.
                LOG_WARNING << "The Trie memory is not allocated efficiently yet! Try using the N-Gram Counts!" << END_LOG;
            }

            template<TModelLevel N>
            void MultiHashMapTrie<N>::add1Gram(const SBackOffNGram &oGram) {
                //First get the token/word from the 1-Gram
                const string & token = oGram.tokens[0];

                LOG_DEBUG << "Adding a 1-Gram: '" << token << "' to the Trie." << END_LOG;

                //Compute it's hash value
                TWordHashSize wordHash = AHashMapTrie<N>::createUniqueIdHash(token);
                //Get the word probability and back-off data reference
                TProbBackOffEntryPair & pbData = oGrams[wordHash];

                //Do a temporary check for hash collisions
                recordAndCheck(wordHash, UNDEFINED_WORD_HASH, oGram);

                //If the probability is not zero then this word has been already seen!
                if (pbData.first != ZERO_LOG_PROB_WEIGHT) {
                    //The word has been seen already, this is a potential error, so we report a warning!
                    REPORT_COLLISION_WARNING(oGram.tokens, wordHash, UNDEFINED_WORD_HASH,
                            pbData.first, pbData.second,
                            oGram.prob, oGram.back_off);
                }

                //Set/Update the probability and back-off values for the word
                pbData.first = oGram.prob;
                pbData.second = oGram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.first << "," << pbData.second << ") for "
                        << ngramToString(oGram.tokens) << " wordHash = "
                        << wordHash << END_LOG;
            }

            template<TModelLevel N>
            void MultiHashMapTrie<N>::addMGram(const SBackOffNGram &mGram) {
                const size_t level = mGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << ngramToString(mGram.tokens) << " to the Trie" << END_LOG;

                //Check that this is not an 1-Gram or N-Gram for those we need another method!
                if ((MIN_NGRAM_LEVEL < level) || (level < N)) {
                    //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                    // 1. Compute the context hash defined by w1 w2 w3
                    TReferenceHashSize contextHash = AHashMapTrie<N>::template computeHashContext<Logger::DEBUG2>(mGram.tokens);

                    // 2. Compute the hash of w4
                    const string & endWord = *(--mGram.tokens.end());
                    TWordHashSize wordHash = AHashMapTrie<N>::getUniqueIdHash(endWord);
                    LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                    // 3. Insert the probability data into the trie
                    //Data stores the N-tires from length 2 on and indexing starts
                    //with 0, therefore "level-2". Get/Create the mapping for this
                    //word in the Trie level of the N-gram
                    TMGramEntryMap& ngamEntry = mGrams[level - 2][wordHash];
                    //Get/Create the new Prob. and Back-Off entry pair in the map, for the context
                    TProbBackOffEntryPair& pbData = ngamEntry[contextHash];

                    //Do a temporary check for hash collisions
                    recordAndCheck(wordHash, contextHash, mGram);

                    //Check that the probability data is not set yet, otherwise a warning!
                    if (pbData.first != ZERO_LOG_PROB_WEIGHT) {
                        //The M-Gram has been seen already, this is a potential error, so we report a warning!
                        REPORT_COLLISION_WARNING(mGram.tokens, wordHash, contextHash,
                                pbData.first, pbData.second,
                                mGram.prob, mGram.back_off);
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

            template<TModelLevel N>
            void MultiHashMapTrie<N>::addNGram(const SBackOffNGram &nGram) {
                const size_t level = nGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << ngramToString(nGram.tokens) << " to the Trie" << END_LOG;

                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TReferenceHashSize contextHash = AHashMapTrie<N>::template computeHashContext<Logger::DEBUG2>(nGram.tokens);

                // 2. Compute the hash of w4
                const string & endWord = *(--nGram.tokens.end());
                TWordHashSize wordHash = AHashMapTrie<N>::getUniqueIdHash(endWord);
                LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                // 3. Insert the probability data into the trie
                //Data stores the N-tires from length 2 on, therefore "idx-1"
                //Get/Create the mapping for this word in the Trie level of the N-gram
                TNGramEntryMap& ngamEntry = nGrams[wordHash];
                //Get/Create the new Prob. in the map, for the context
                TLogProbBackOff& pData = ngamEntry[contextHash];

                //Do a temporary check for hash collisions
                recordAndCheck(wordHash, contextHash, nGram);

                //Check that the probability data is not set yet, otherwise a warning!
                if (pData != ZERO_LOG_PROB_WEIGHT) {
                    //The M-Gram has been seen already, this is a potential error, so we report a warning!
                    REPORT_COLLISION_WARNING(nGram.tokens, wordHash, contextHash,
                            pData, UNDEFINED_LOG_PROB_WEIGHT,
                            nGram.prob, UNDEFINED_LOG_PROB_WEIGHT);
                }

                //Set/Update the probability
                pData = nGram.prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << pData << ") for "
                        << ngramToString(nGram.tokens) << " contextHash = "
                        << contextHash << ", wordHash = " << wordHash << END_LOG;
            }

            template<TModelLevel N>
            TLogProbBackOff MultiHashMapTrie<N>::getBackOffWeight(const TModelLevel contextLength) {
                //Get the word hash for the en word of the back-off N-Gram
                const TWordHashSize & endWordHash = AHashMapTrie<N>::getBackOffNGramEndWordHash();
                const TModelLevel backOfContextLength = contextLength - 1;
                //Set the initial back-off weight value to undefined!
                TLogProbBackOff back_off = ZERO_LOG_PROB_WEIGHT;

                LOG_DEBUG1 << "Computing back-off for an " << (backOfContextLength + 1)
                        << "-gram the context length is " << backOfContextLength << END_LOG;

                if (backOfContextLength > 0) {
                    //Compute the context hash
                    TReferenceHashSize contextHash = AHashMapTrie<N>::computeHashContext(backOfContextLength, true);
                    //Attempt to retrieve back-off weights
                    try {
                        //The context length plus one is M value of the M-Gram
                        //All the M-grams for 1 < M < N are stored in a mGrams
                        //array, so this M-Gram is stored under index M-2;
                        const TModelLevel mGramIdx = ((backOfContextLength + 1) - 2);
                        TProbBackOffEntryPair & entry = mGrams[mGramIdx].at(endWordHash).at(contextHash);

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
                } else {
                    //We came to a zero context, which means we have an
                    //1-Gram to try to get the back-off weight from

                    //Attempt to retrieve back-off weights
                    try {
                        TProbBackOffEntryPair & pbData = oGrams.at(endWordHash);
                        //Note that: If the stored back-off is UNDEFINED_LOG_PROB_WEIGHT then the back of is just zero
                        back_off = pbData.second;

                        LOG_DEBUG2 << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( back-off ) for word: " << endWordHash
                                << ", is: " << back_off << END_LOG;
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: " << endWordHash << ", nowhere to back-off!" << END_LOG;
                    }
                }

                LOG_DEBUG2 << "The chosen log back-off weight for context: " << contextLength << " is: " << back_off << END_LOG;

                //Return the computed back-off weight it can be UNDEFINED_LOG_PROB_WEIGHT, which is zero - no penalty
                return back_off;
            }

            template<TModelLevel N>
            TLogProbBackOff MultiHashMapTrie<N>::computeLogProbability(const TModelLevel contextLength) {
                //Get the last word in the N-gram
                const TWordHashSize & endWordHash = AHashMapTrie<N>::getNGramEndWordHash();

                LOG_DEBUG1 << "Computing probability for an " << (contextLength + 1)
                        << "-gram the context length is " << contextLength << END_LOG;

                //Consider different variants based no the length of the context
                if (contextLength > 0) {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram

                    //Compute the context hash based on what is stored in _wordHashes and context length
                    TReferenceHashSize contextHash = AHashMapTrie<N>::computeHashContext(contextLength, false);

                    //Attempt to retrieve probabilities
                    try {
                        if (contextLength == (N - 1)) {
                            //If we are looking for a N-Gram probability
                            TLogProbBackOff & prob = nGrams.at(endWordHash).at(contextHash);

                            LOG_DEBUG2 << "The " << N << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                    << "( prob. ) for (word,context) = (" << endWordHash << ", "
                                    << contextHash << "), is: " << prob << END_LOG;

                            //Return the stored probability
                            return prob;
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N

                            //The context length plus one is M value of the M-Gram
                            //All the M-grams for 1 < M < N are stored in a mGrams
                            //array, so this M-Gram is stored under index M-2;
                            const TModelLevel mGramIdx = ((contextLength + 1) - 2);
                            //Get the probability/back-off entry for the given M-gram
                            TProbBackOffEntryPair & entry = mGrams[mGramIdx].at(endWordHash).at(contextHash);

                            LOG_DEBUG2 << "The " << (contextLength + 1)
                                    << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                    << "( prob. ) for (word,context) = ("
                                    << endWordHash << ", " << contextHash
                                    << "), is: " << entry.first << END_LOG;

                            //Return the stored probability
                            return entry.first;
                        }
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the " << (contextLength + 1)
                                << "-Gram  prob for a (word,context) = ("
                                << endWordHash << ", " << contextHash
                                << "), need to back off!" << END_LOG;

                        const TLogProbBackOff back_off = getBackOffWeight(contextLength);
                        const TLogProbBackOff probability = computeLogProbability(contextLength - 1);

                        LOG_DEBUG1 << "getBackOffWeight(" << contextLength << ") = " << back_off
                                << ", computeLogProbability(" << (contextLength - 1) << ") = "
                                << probability << END_LOG;

                        LOG_DEBUG2 << "The " << contextLength << " probability = " << back_off
                                << " + " << probability << " = " << (back_off + probability) << END_LOG;

                        //Do the back-off weight plus the lower level probability, we do a plus as we work with LOG probabilities
                        return back_off + probability;
                    }
                } else {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    try {
                        TProbBackOffEntryPair & pbData = oGrams.at(endWordHash);

                        LOG_DEBUG2 << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( prob. ) for word: " << endWordHash
                                << ", is: " << pbData.first << END_LOG;

                        //Return the stored probability
                        return pbData.first;
                    } catch (out_of_range e) {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << endWordHash << " returning:" << MINIMAL_LOG_PROB_WEIGHT
                                << " for log_" << LOG_PROB_WEIGHT_BASE << "( prob. )" << END_LOG;

                        //Return the default minimal probability for an unknown word
                        return MINIMAL_LOG_PROB_WEIGHT;
                    }
                }
            }

            template<TModelLevel N>
            void MultiHashMapTrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result) {
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
            MultiHashMapTrie<N>::MultiHashMapTrie(const MultiHashMapTrie& orig) {
            }

            template<TModelLevel N>
            MultiHashMapTrie<N>::~MultiHashMapTrie() {
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class MultiHashMapTrie<MAX_NGRAM_LEVEL>;
        }
    }
}