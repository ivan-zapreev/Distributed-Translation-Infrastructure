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

#include <vector>       //std::vector
#include <string>       //std::string
#include <functional>   // std::function 

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
             * This is a function type for the function that should be able to
             * provide a new (next) context id for a word id and a previous context.
             * @param wordId the word id
             * @param ctxId the context id
             * @param level the M-gram level we are working with, must have M > 1 or UNDEF_NGRAM_LEVEL!
             * @result the new context id
             */
            typedef std::function<TContextId(const TWordId wordId, const TContextId ctxId, const TModelLevel level) > TGetCtxIdFunct;

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
                explicit ATrie(AWordIndex * const _pWordIndex, TGetCtxIdFunct get_ctx_id_func)
                : m_p_word_index(_pWordIndex), m_get_ctx_id_func(get_ctx_id_func),
                m_chached_context(m_context_c_str, MAX_N_GRAM_STRING_LENGTH),
                m_chached_context_id(UNDEFINED_WORD_ID) {
                    //This one is needed for having a proper non-null word index pointer.
                    if (_pWordIndex == NULL) {
                        stringstream msg;
                        msg << "Unable to use " << __FILE__ << ", the word index pointer must not be NULL!";
                        throw Exception(msg.str());
                    }
                };

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the array of N-Gram counts counts[0] is for 1-Gram
                 */
                virtual void preAllocate(const size_t counts[N]) {
                    //Compute the number of words to be stored
                    //Add an extra element for the <unknown/> word
                    m_p_word_index->reserve(counts[0] + 1);
                };

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
                    return m_p_word_index;
                }

                /**
                 * The basic class destructor
                 */
                virtual ~ATrie() {
                };

            protected:

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                ATrie(const ATrie& orig) : m_p_word_index(NULL), m_get_ctx_id_func(NULL), m_chached_context(),
                m_chached_context_id(UNDEFINED_WORD_ID) {
                    throw Exception("ATrie copy constructor is not to be used, unless implemented!");
                };

                /**
                 * Gets the word hash for the end word of the back-off N-Gram
                 * @return the word hash for the end word of the back-off N-Gram
                 */
                inline const TWordId & getBackOffNGramEndWordHash() {
                    return mGramWordIds[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the N-gram
                 * @return the word hash for the last word in the N-gram
                 */
                inline const TWordId & getNGramEndWordHash() {
                    return mGramWordIds[N - 1];
                }

                /**
                 * This method converts the M-Gram tokens into hashes and stores
                 * them in an array. Note that, M is the size of the tokens array.
                 * It is not checked, for the sake of performance but is assumed
                 * that M is <= N!
                 * @param tokens the tokens to be transformed into word hashes must have size <=N
                 * @param wordHashes the out array parameter to store the hashes.
                 */
                inline void tokensToHashes(const vector<string> & tokens, TWordId wordHashes[N]) {
                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - tokens.size();
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(tokens.size()) << "-gram:" << END_LOG;
                    for (vector<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                        wordHashes[idx] = m_p_word_index->getId(*it);
                        LOG_DEBUG1 << "hash('" << *it << "') = " << SSTR(wordHashes[idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * Converts the given tokens to hashes and stores it in mGramWordHashes
                 * @param ngram the n-gram tokens to convert to hashes
                 */
                inline void storeNGramHashes(const vector<string> & ngram) {
                    //First transform the given M-gram into word hashes.
                    tokensToHashes(ngram, mGramWordIds);
                }

                /**
                 * Compute the context hash for the M-Gram prefix, example:
                 * 
                 *  N = 5
                 * 
                 *   0  1  2  3  4
                 *  w1 w2 w3 w4 w5
                 * 
                 *  contextLength = 2
                 * 
                 *    0  1  2  3  4
                 *   w1 w2 w3 w4 w5
                 *          ^  ^
                 * Hash will be computed for the 3-gram prefix w3 w4.
                 * 
                 * @param contextLength the length of the context to compute
                 * @param isBackOff is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @return the computed hash context
                 */
                inline TContextId getQueryContextId(const TModelLevel contextLength, bool isBackOff) {
                    const TModelLevel mGramEndIdx = (isBackOff ? (N - 2) : (N - 1));
                    const TModelLevel eIdx = mGramEndIdx;
                    const TModelLevel bIdx = mGramEndIdx - contextLength;
                    TModelLevel idx = bIdx;

                    LOG_DEBUG3 << "Computing context hash for context length " << SSTR(contextLength)
                            << " for a  " << (isBackOff ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    TContextId ctxId = mGramWordIds[idx];
                    LOG_DEBUG3 << "Word: " << SSTR(idx) << " id == initial context id: " << SSTR(ctxId) << END_LOG;
                    idx++;

                    //Compute the subsequent context ids
                    for (; idx < eIdx;) {
                        ctxId = m_get_ctx_id_func(mGramWordIds[idx], ctxId, (idx - bIdx) + 1);
                        LOG_DEBUG3 << "Idx: " << SSTR(idx) << ", getContextId(" << SSTR(mGramWordIds[idx]) << ", prevContextId) = " << SSTR(ctxId) << END_LOG;
                        idx++;
                    }

                    LOG_DEBUG3 << "Resulting context hash for context length " << SSTR(contextLength)
                            << " of a  " << (isBackOff ? "back-off" : "probability")
                            << " computation is: " << SSTR(ctxId) << END_LOG;

                    return ctxId;
                }

                /**
                 * This function computes the hash context of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must not be called on M-grams with M <= 1!
                 * 
                 * @param gram the N-gram with its tokens to create context for
                 * @return the resulting hash of the context(w1 w2 w3)
                 */
                template<DebugLevel logLevel>
                inline TContextId getContextId(const SRawNGram & gram) {
                    TContextId ctxId;

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (ATrie<N>::getCachedContextId(gram, ctxId)) {
                        //Get the start context value for the first token
                        const string & token = gram.tokens[0].str();

                        //There is no id cached for this M-gram context - compute it
                        ctxId = ATrie<N>::getWordIndex()->getId(token);

                        LOGGER(logLevel) << "ctxId = getId('" << token << "') = " << SSTR(ctxId) << END_LOG;

                        //Iterate and compute the hash:
                        for (int i = 1; i < (gram.level - 1); i++) {
                            const string & token = gram.tokens[i].str();
                            const TWordId wordId = ATrie<N>::getWordIndex()->getId(token);
                            LOGGER(logLevel) << "wordId = getId('" << token << "') = " << SSTR(wordId) << END_LOG;
                            ctxId = m_get_ctx_id_func(wordId, ctxId, i + 1);
                            LOGGER(logLevel) << "ctxId = createContext( wordId, ctxId ) = " << SSTR(ctxId) << END_LOG;
                        }

                        //Cache the newly computed context id for the given n-gram context
                        ATrie<N>::cacheContextId(gram, ctxId);
                    }

                    return ctxId;
                }

                /**
                 * Allows to retrieve the cached context id for the given M-gram if any
                 * @param mGram the m-gram to get the context id for
                 * @param result the output parameter, will store the cached id, if any
                 * @return true if there was nothing cached, otherwise false
                 */
                inline bool getCachedContextId(const SRawNGram &mGram, TContextId & result) {
                    if (m_chached_context == mGram.context) {
                        result = m_chached_context_id;
                        LOG_DEBUG3 << "Cache match! " << m_chached_context << " == " << mGram.context << END_LOG;
                        return false;
                    }
                    return true;
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void cacheContextId(const SRawNGram &mGram, TContextId & stx_id) {
                    m_chached_context.copy_string<MAX_N_GRAM_STRING_LENGTH>(mGram.context);
                    m_chached_context_id = stx_id;
                    LOG_DEBUG3 << "Caching context = [ " << m_chached_context << " ], id = " << m_chached_context_id << END_LOG;
                }

            private:
                //Stores the reference to the word index to be used
                AWordIndex * const m_p_word_index;

                //Stores the pointer to the function that will be used to compute
                //the context id from a word id and the previous context
                TGetCtxIdFunct m_get_ctx_id_func;

                //The actual storage for the cached context c string
                char m_context_c_str[MAX_N_GRAM_STRING_LENGTH];
                //Stores the cached M-gram context (for 1 < M <= N )
                TextPieceReader m_chached_context;
                //Stores the cached M-gram context value (for 1 < M <= N )
                TContextId m_chached_context_id;

                //The temporary data structure to store the N-gram query word hashes
                TWordId mGramWordIds[N];
            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<MAX_NGRAM_LEVEL> TFiveTrie;

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ATrie<MAX_NGRAM_LEVEL>;
        }
    }
}
#endif	/* ITRIES_HPP */

