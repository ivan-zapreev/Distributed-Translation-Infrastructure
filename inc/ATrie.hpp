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

#include <vector>       // std::vector
#include <string>       // std::string
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
                TLogProbBackOff prob;
            };

            /**
             * This is a function type for the function that should be able to
             * provide a new (next) context id for a word id and a previous context.
             * 
             * WARNING: Must only be called for the M-gram level 1 < M <= N!
             * 
             * @param wordId the word id
             * @param ctxId the in/out parameter that is a context id, the input is the previous context id, the output is the next context id
             * @param level the M-gram level we are working with, must have 1 < M <= N or UNDEF_NGRAM_LEVEL!
             * @result true if the next context id could be computed, otherwise false
             * @throw nothign
             */
            typedef std::function<bool (const TShortId wordId, TLongId & ctxId, const TModelLevel level) > TGetCtxIdFunct;

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
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                const static TShortId EXTRA_NUMBER_OF_WORD_IDs = 2;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;
                
                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = N - MGRAM_IDX_OFFSET;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit ATrie(AWordIndex * const _pWordIndex,
                        TGetCtxIdFunct get_ctx_id_func)
                : m_p_word_index(_pWordIndex),
                m_get_ctx_id_func(get_ctx_id_func),
                m_chached_ctx(m_context_c_str, MAX_N_GRAM_STRING_LENGTH),
                m_chached_ctx_id(UNDEFINED_WORD_ID) {
                    //Clear the memory for the buffer and initialize it
                    memset(m_context_c_str, 0, MAX_N_GRAM_STRING_LENGTH * sizeof (char));
                    m_context_c_str[0] = '\0';

                    //This one is needed for having a proper non-null word index pointer.
                    if (_pWordIndex == NULL) {
                        stringstream msg;
                        msg << "Unable to use " << __FILE__ << ", the word index pointer must not be NULL!";
                        throw Exception(msg.str());
                    }

                    LOG_INFO3 << "Collision detections are: "
                            << (DO_SANITY_CHECKS ? "ON" : "OFF")
                            << " !" << END_LOG;
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
                void add_1_Gram(const SRawNGram &oGram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param mGram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                void add_M_Gram(const SRawNGram &mGram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param nGram the N-Gram data
                 */
                void add_N_Gram(const SRawNGram &nGram);

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
                void queryNGram(const SRawNGram & ngram, SProbResult & result);

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

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * @param wordId the One-gram id
                 * @return the reference to the storage structure
                 */
                virtual TProbBackOffEntry & make_1_GramDataRef(const TShortId wordId) = 0;

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param wordId the One-gram id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                virtual bool get_1_GramDataRef(const TShortId wordId, const TProbBackOffEntry ** ppData) = 0;

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * @param level the value of M in the M-gram
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @return the reference to the storage structure
                 */
                virtual TProbBackOffEntry& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, TLongId ctxId) = 0;

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param level the value of M in the M-gram
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                virtual bool get_M_GramDataRef(const TModelLevel level, const TShortId wordId,
                        TLongId ctxId, const TProbBackOffEntry **ppData) = 0;

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * @param wordId the id of the N-gram's last word
                 * @param ctxId the N-gram context (the N-gram's prefix) id
                 * @return the reference to the storage structure
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) = 0;

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * @param wordId the id of the N-gram's last word
                 * @param ctxId the N-gram context (the N-gram's prefix) id
                 * @param ppData[out] the pointer to a pointer to the found data
                 * @return true if the probability was found, otherwise false
                 * @throw nothing
                 */
                virtual bool get_N_GramProb(const TShortId wordId, const TLongId ctxId,
                        TLogProbBackOff & prob) = 0;

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                ATrie(const ATrie& orig)
                : m_p_word_index(NULL),
                m_get_ctx_id_func(NULL),
                m_chached_ctx(),
                m_chached_ctx_id(UNDEFINED_WORD_ID) {
                    throw Exception("ATrie copy constructor is not to be used, unless implemented!");
                };

                /**
                 * Gets the word hash for the end word of the back-off N-Gram
                 * @return the word hash for the end word of the back-off N-Gram
                 */
                inline const TShortId & getBackOffNGramEndWordHash() {
                    return m_GramWordIds[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the N-gram
                 * @return the word hash for the last word in the N-gram
                 */
                inline const TShortId & getNGramEndWordHash() {
                    return m_GramWordIds[N - 1];
                }

                /**
                 * This method converts the M-Gram tokens into hashes and stores
                 * them in an array. Note that, M is the size of the tokens array.
                 * It is not checked, for the sake of performance but is assumed
                 * that M is <= N!
                 * @param ngram the n-gram structure with the query ngram tokens
                 * @param wordHashes the out array parameter to store the hashes.
                 */
                inline void tokensToId(const SRawNGram & ngram, TShortId wordHashes[N]) {
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
                 * Converts the given tokens to hashes and stores it in mGramWordHashes
                 * @param ngram the n-gram tokens to convert to hashes
                 */
                inline void storeNGramHashes(const SRawNGram & ngram) {
                    //First transform the given M-gram into word hashes.
                    tokensToId(ngram, m_GramWordIds);
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
                 * @param ctxLen the length of the context to compute
                 * @param isBackOff is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @param ctxId [out] the context id to be computed
                 * @return the true if the context could be computed, otherwise false
                 * @throws nothing
                 */
                template<bool isBackOff>
                inline bool getQueryContextId(const TModelLevel ctxLen, TLongId & ctxId) {
                    const TModelLevel mGramEndIdx = (isBackOff ? (N - 2) : (N - 1));
                    const TModelLevel eIdx = mGramEndIdx;
                    const TModelLevel bIdx = mGramEndIdx - ctxLen;
                    TModelLevel idx = bIdx;

                    LOG_DEBUG1 << "Computing ctxId for context length: " << SSTR(ctxLen)
                            << " for a  " << (isBackOff ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    ctxId = m_GramWordIds[idx];
                    LOG_DEBUG1 << "First word @ idx: " << SSTR(idx) << " has wordId: " << SSTR(ctxId) << END_LOG;
                    idx++;

                    //Since the first word defines the second word context, and
                    //if this word is unknown then there is definitely no data
                    //for this N-gram in the trie ... so we through!
                    if (ctxId < MIN_KNOWN_WORD_ID) {
                        LOG_DEBUG1 << "The first " << SSTR(ctxLen + 1) << " wordId == "
                                << SSTR(ctxId) << " i.e. <unk>, need to back-off!" << END_LOG;
                        return false;
                    } else {
                        //Compute the subsequent context ids
                        for (; idx < eIdx;) {
                            LOG_DEBUG1 << "Start searching ctxId for mGramWordIds[" << SSTR(idx) << "]: "
                                    << SSTR(m_GramWordIds[idx]) << " prevCtxId: " << SSTR(ctxId) << END_LOG;
                            if (m_get_ctx_id_func(m_GramWordIds[idx], ctxId, (idx - bIdx) + 1)) {
                                LOG_DEBUG1 << "getContextId(" << SSTR(m_GramWordIds[idx])
                                        << ", prevCtxId) = " << SSTR(ctxId) << END_LOG;
                                idx++;
                            } else {
                                //The next context id could not be retrieved
                                return false;
                            }
                        }

                        LOG_DEBUG1 << "Resulting context hash for context length " << SSTR(ctxLen)
                                << " of a  " << (isBackOff ? "back-off" : "probability")
                                << " computation is: " << SSTR(ctxId) << END_LOG;

                        return true;
                    }
                }

                /**
                 * This function computes the context id of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must be called on M-grams with M > 1!
                 * 
                 * @param gram the N-gram with its tokens to create context for
                 * @param the resulting hash of the context(w1 w2 w3)
                 * @return true if the context was found otherwise false
                 */
                template<DebugLevelsEnum logLevel>
                inline bool getContextId(const SRawNGram & gram, TLongId &ctxId) {
                    //Try to retrieve the context from the cache, if not present then compute it
                    if (getCachedContextId(gram, ctxId)) {
                        //Get the start context value for the first token
                        const string & token = gram.tokens[0].str();
                        TShortId wordId;

                        //There is no id cached for this M-gram context - find it
                        if (m_p_word_index->getId(token, wordId)) {
                            //The first word id is the first context id
                            ctxId = wordId;
                            LOGGER(logLevel) << "ctxId = getId('" << token
                                    << "') = " << SSTR(ctxId) << END_LOG;

                            //Iterate and compute the hash:
                            for (int i = 1; i < (gram.level - 1); i++) {
                                const string & token = gram.tokens[i].str();
                                if (m_p_word_index->getId(token, wordId)) {
                                    LOGGER(logLevel) << "wordId = getId('" << token
                                            << "') = " << SSTR(wordId) << END_LOG;
                                    if (m_get_ctx_id_func(wordId, ctxId, i + 1)) {
                                        LOGGER(logLevel) << "ctxId = computeCtxId( "
                                                << "wordId, ctxId ) = " << SSTR(ctxId) << END_LOG;
                                    } else {
                                        //The next context id could not be computed
                                        LOGGER(logLevel) << "The next context for wordId: "
                                                << SSTR(wordId) << " and ctxId: "
                                                << SSTR(ctxId) << "on level: " << SSTR((i + 1))
                                                << "could not be computed!" << END_LOG;
                                        return false;
                                    }
                                } else {
                                    //The next word Id was not found, it is
                                    //unknown, so we can stop searching
                                    LOGGER(logLevel) << "The wordId for '" << token
                                            << "' could not be found!" << END_LOG;
                                    return false;
                                }
                            }

                            //Cache the newly computed context id for the given n-gram context
                            setCacheContextId(gram, ctxId);

                            //The context Id was found in the Trie
                            LOGGER(logLevel) << "The ctxId could be computed, "
                                    << "it's value is: " << SSTR(ctxId) << END_LOG;
                            return true;
                        } else {
                            //The context id could not be computed as
                            //the first N-gram's word is already unknown
                            LOGGER(logLevel) << "The wordId for '" << token
                                    << "' could not be found!" << END_LOG;
                            return false;
                        }
                    } else {
                        //The context Id was found in the cache
                        LOGGER(logLevel) << "The ctxId was found in cache, "
                                << "it's value is: " << SSTR(ctxId) << END_LOG;
                        return true;
                    }
                }

                /**
                 * Allows to retrieve the cached context id for the given M-gram if any
                 * @param mGram the m-gram to get the context id for
                 * @param result the output parameter, will store the cached id, if any
                 * @return true if there was nothing cached, otherwise false
                 */
                inline bool getCachedContextId(const SRawNGram &mGram, TLongId & result) {
                    if (m_chached_ctx == mGram.context) {
                        result = m_chached_ctx_id;
                        LOG_DEBUG2 << "Cache MATCH! [" << m_chached_ctx << "] == [" << mGram.context
                                << "], for m-gram: " << tokensToString<N>(mGram.tokens, mGram.level)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return false;
                    } else {
                        LOG_DEBUG2 << "Cache MISS! [" << m_chached_ctx << "] != [" << mGram.context
                                << "], for m-gram: " << tokensToString<N>(mGram.tokens, mGram.level)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return true;
                    }
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void setCacheContextId(const SRawNGram &mGram, TLongId & stx_id) {
                    LOG_DEBUG2 << "Caching context = [ " << mGram.context << " ], id = " << stx_id
                            << ", for m-gram: " << tokensToString<N>(mGram.tokens, mGram.level) << END_LOG;

                    m_chached_ctx.copy_string<MAX_N_GRAM_STRING_LENGTH>(mGram.context);
                    m_chached_ctx_id = stx_id;

                    LOG_DEBUG2 << "Cached context = [ " << m_chached_ctx
                            << " ], id = " << SSTR(m_chached_ctx_id) << END_LOG;
                }

            private:
                //Stores the reference to the word index to be used
                AWordIndex * const m_p_word_index;

                //Stores the pointer to the function that will be used to compute
                //the context id from a word id and the previous context,
                //Throws out_of_range in case the context can not be computed, e.g. does not exist.
                TGetCtxIdFunct m_get_ctx_id_func;

                //The actual storage for the cached context c string
                char m_context_c_str[MAX_N_GRAM_STRING_LENGTH];
                //Stores the cached M-gram context (for 1 < M <= N )
                TextPieceReader m_chached_ctx;
                //Stores the cached M-gram context value (for 1 < M <= N )
                TLongId m_chached_ctx_id;

                //The temporary data structure to store the N-gram query word ids
                TShortId m_GramWordIds[N];

                /**
                 * This function should be called in case we can not get the probability for
                 * the given M-gram and we want to compute it's back-off probability instead
                 * @param ctxLen the length of the context for the M-gram for which we could
                 * not get the probability from the trie.
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                inline void getProbabilityBackOff(const TModelLevel ctxLen, TLogProbBackOff & prob) {
                    //Compute the lover level probability
                    getProbability(ctxLen, prob);

                    LOG_DEBUG1 << "getProbability(" << ctxLen
                            << ") = " << prob << END_LOG;

                    //If the probability is not zero then go on with computing the
                    //back-off. Otherwise it does not make sence to compute back-off.
                    if (prob > ZERO_LOG_PROB_WEIGHT) {
                        TLogProbBackOff back_off;
                        if (!getBackOffWeight(ctxLen, back_off)) {
                            //Set the back-off weight value to zero as there is no back-off found!
                            back_off = ZERO_BACK_OFF_WEIGHT;
                        }

                        LOG_DEBUG1 << "getBackOffWeight(" << ctxLen
                                << ") = " << back_off << END_LOG;

                        LOG_DEBUG2 << "The " << ctxLen << " probability = " << back_off
                                << " + " << prob << " = " << (back_off + prob) << END_LOG;

                        //Do the back-off weight plus the lower level probability, we do a plus as we work with LOG probabilities
                        prob += back_off;
                    }
                }

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes are obtained from the _wordHashes member
                 * variable of the class. So it must be pre-set with proper
                 * word hash values first!
                 * @param level the M-gram level for which the probability is to be computed
                 * @param prob [out] the reference to the probability to be found/computed
                 */
                void getProbability(const TModelLevel level, TLogProbBackOff & prob);

                /**
                 * This recursive function allows to get the back-off weight for the current context.
                 * The N-Gram hashes are obtained from the pre-computed data member array _wordHashes
                 * @param level the M-gram level for which the back-off weight is to be found,
                 * is equal to the context length of the K-Gram in the caller function
                 * @param back_off [out] the back-off weight to be computed
                 * @return the resulting back-off weight probability
                 */
                bool getBackOffWeight(const TModelLevel level, TLogProbBackOff & back_off);

            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<MAX_NGRAM_LEVEL> TFiveTrie;
        }
    }
}
#endif	/* ITRIES_HPP */

