/* 
 * File:   ContextContextMultiHashMapTrie.cpp
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
 * Created on August 14, 2015, 1:53 PM
 */
#include "ContextMultiHashMapTrie.hpp"

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
            ContextMultiHashMapTrie<N>::ContextMultiHashMapTrie(const float _wordIndexMemFactor,
                    const float _oGramMemFactor,
                    const float _mGramMemFactor,
                    const float _nGramMemFactor)
            : AHashMapTrie<N>(_wordIndexMemFactor), 
            oGramMemFactor(_oGramMemFactor),
            mGramMemFactor(_mGramMemFactor),
            nGramMemFactor(_nGramMemFactor) {
                //Perform an error check! This container has a lower bound on the N level.
                if (N < BGRAM_LEVEL_VALUE) {
                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << BGRAM_LEVEL_VALUE << "!";
                    throw Exception(msg.str());
                }

                LOG_INFO << "Using the " << __FILE__ << " model!" << END_LOG;
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::preAllocateOGrams(const size_t counts[N]) {
                //Compute the number of words to be stored
                const size_t numEntries = counts[0] + 1; //Add an extra element for the <unknown/> word

                //Reserve the memory for the map
                reserve_mem_unordered_map<TOneGramsMap, TOneGramAllocator>(&pOneGramMap, &pOneGramAlloc, numEntries, "1-Grams", oGramMemFactor);

                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntryPair & pbData = pOneGramMap->operator[](UNKNOWN_WORD_HASH);
                pbData.first = MINIMAL_LOG_PROB_WEIGHT;
                pbData.second = UNDEFINED_LOG_PROB_WEIGHT;
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 1; idx < (N - 1); idx++) {
                    //Get the number of elements to pre-allocate
                    const uint numEntries = counts[idx];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx - 1], &pMGramAlloc[idx - 1], numEntries, "M-Grams", mGramMemFactor);
                }
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate
                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", nGramMemFactor);
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::preAllocate(const size_t counts[N]) {
                //Call the super class pre-allocator!
                AHashMapTrie<N>::preAllocate(counts);

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::add1Gram(const SBackOffNGram &oGram) {
                //First get the token/word from the 1-Gram
                const string & token = oGram.tokens[0];

                LOG_DEBUG << "Adding a 1-Gram: '" << token << "' to the Trie." << END_LOG;

                //Compute it's hash value
                TWordHashSize wordHash = AHashMapTrie<N>::createUniqueIdHash(token);
                //Get the word probability and back-off data reference
                TProbBackOffEntryPair & pbData = pOneGramMap->operator[](wordHash);

#if MONITORE_COLLISIONS
                //Do a temporary check for hash collisions
                AHashMapTrie<N>::recordAndCheck(wordHash, UNDEFINED_WORD_HASH, oGram);
                //If the probability is not zero then this word has been already seen!
                if (pbData.first != ZERO_LOG_PROB_WEIGHT) {
                    //The word has been seen already, this is a potential error, so we report a warning!
                    REPORT_COLLISION_WARNING(oGram.tokens, wordHash, UNDEFINED_WORD_HASH,
                            pbData.first, pbData.second,
                            oGram.prob, oGram.back_off);
                }
#endif
                //Set/Update the probability and back-off values for the word
                pbData.first = oGram.prob;
                pbData.second = oGram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.first << "," << pbData.second << ") for "
                        << tokensToString(oGram.tokens) << " wordHash = "
                        << wordHash << END_LOG;
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::addMGram(const SBackOffNGram &mGram) {
                const TModelLevel level = mGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << tokensToString(mGram.tokens) << " to the Trie" << END_LOG;

                //Check that this is not an 1-Gram or N-Gram for those we need another method!
                if ((MIN_NGRAM_LEVEL < level) || (level < N)) {
                    //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                    // 1. Compute the context hash defined by w1 w2 w3
                    const TReferenceHashSize contextHash = AHashMapTrie<N>::template computeHashContext<Logger::DEBUG2>(mGram.tokens);

                    // 2. Compute the hash of w4
                    const string & endWord = *(--mGram.tokens.end());
                    const TWordHashSize wordHash = AHashMapTrie<N>::getUniqueIdHash(endWord);
                    LOG_DEBUG2 << "wordHash = computeHash('" << endWord << "') = " << wordHash << END_LOG;

                    // 3. Insert the probability data into the trie
                    // 3. Insert the probability data into the trie
                    //Store the N-tires from length 2 on and indexing starts
                    //with 0, therefore "level-2". Get/Create the mapping for this
                    //word in the Trie level of the N-gram
                    TReferenceHashSize keyContext = AHashMapTrie<N>::createContext(wordHash, contextHash);
                    TProbBackOffEntryPair& pbData = pMGramMap[level - MGRAM_IDX_OFFSET]->operator[](keyContext);

#if MONITORE_COLLISIONS
                    //Do a temporary check for hash collisions
                    AHashMapTrie<N>::recordAndCheck(wordHash, contextHash, mGram);
                    //Check that the probability data is not set yet, otherwise a warning!
                    if (pbData.first != ZERO_LOG_PROB_WEIGHT) {
                        //The M-Gram has been seen already, this is a potential error, so we report a warning!
                        REPORT_COLLISION_WARNING(mGram.tokens, wordHash, contextHash,
                                pbData.first, pbData.second,
                                mGram.prob, mGram.back_off);
                    }
#endif
                    //Set/Update the probability and back-off values for the word
                    pbData.first = mGram.prob;
                    pbData.second = mGram.back_off;

                    LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                            << pbData.first << "," << pbData.second << ") for "
                            << tokensToString(mGram.tokens) << " contextHash = "
                            << contextHash << ", wordHash = " << wordHash << END_LOG;
                } else {
                    stringstream msg;
                    msg << "Internal error: The " << level << "-Grams are to be handled with another add method!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N>
            void ContextMultiHashMapTrie<N>::addNGram(const SBackOffNGram &nGram) {
                const size_t level = nGram.tokens.size();
                LOG_DEBUG << "Adding a " << level << "-Gram " << tokensToString(nGram.tokens) << " to the Trie" << END_LOG;

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
                TReferenceHashSize keyContext = AHashMapTrie<N>::createContext(wordHash, contextHash);
                TLogProbBackOff& pData = pNGramMap->operator[](keyContext);

#if MONITORE_COLLISIONS
                //Do a temporary check for hash collisions
                AHashMapTrie<N>::recordAndCheck(wordHash, contextHash, nGram);
                //Check that the probability data is not set yet, otherwise a warning!
                if (pData != ZERO_LOG_PROB_WEIGHT) {
                    //The M-Gram has been seen already, this is a potential error, so we report a warning!
                    REPORT_COLLISION_WARNING(nGram.tokens, wordHash, contextHash,
                            pData, UNDEFINED_LOG_PROB_WEIGHT,
                            nGram.prob, UNDEFINED_LOG_PROB_WEIGHT);
                }
#endif
                //Set/Update the probability
                pData = nGram.prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << pData << ") for "
                        << tokensToString(nGram.tokens) << " contextHash = "
                        << contextHash << ", wordHash = " << wordHash << END_LOG;
            }

            template<TModelLevel N>
            TLogProbBackOff ContextMultiHashMapTrie<N>::getBackOffWeight(const TModelLevel contextLength) {
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
                        TReferenceHashSize keyContext = AHashMapTrie<N>::createContext(endWordHash, contextHash);
                        TProbBackOffEntryPair & entry = pMGramMap[(backOfContextLength + 1) - MGRAM_IDX_OFFSET]->at(keyContext);

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
                        TProbBackOffEntryPair & pbData = pOneGramMap->at(endWordHash);
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
            TLogProbBackOff ContextMultiHashMapTrie<N>::computeLogProbability(const TModelLevel contextLength) {
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
                        TReferenceHashSize keyContext = AHashMapTrie<N>::createContext(endWordHash, contextHash);
                        if (contextLength == (N - 1)) {
                            //If we are looking for a N-Gram probability
                            TLogProbBackOff & prob = pNGramMap->at(keyContext);

                            LOG_DEBUG2 << "The " << N << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                    << "( prob. ) for (word,context) = (" << endWordHash << ", "
                                    << contextHash << "), is: " << prob << END_LOG;

                            //Return the stored probability
                            return prob;
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N

                            //The context length plus one is M value of the M-Gram
                            TProbBackOffEntryPair & entry = pMGramMap[(contextLength + 1) - MGRAM_IDX_OFFSET]->at(keyContext);

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
                        TProbBackOffEntryPair & pbData = pOneGramMap->at(endWordHash);

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
            void ContextMultiHashMapTrie<N>::queryNGram(const vector<string> & ngram, SProbResult & result) {
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
            ContextMultiHashMapTrie<N>::~ContextMultiHashMapTrie() {
                //Deallocate One-Grams
                deallocate_container<TOneGramsMap, TOneGramAllocator>(&pOneGramMap, &pOneGramAlloc);

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < (N - 2); idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx]);
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ContextMultiHashMapTrie<MAX_NGRAM_LEVEL>;
        }
    }
}