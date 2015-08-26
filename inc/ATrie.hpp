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
             * 
             * WARNING: Must only be called for the M-gram level 1 < M <= N!
             * 
             * @param wordId the word id
             * @param ctxId the context id
             * @param level the M-gram level we are working with, must have 1 < M <= N or UNDEF_NGRAM_LEVEL!
             * @result the new context id
             * @throw out_of_range in case the context can not be computed, e.g. does not exist.
             */
            typedef std::function<TLongId(const TShortId wordId, const TLongId ctxId, const TModelLevel level) > TGetCtxIdFunct;

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
                explicit ATrie(AWordIndex * const _pWordIndex,
                        TGetCtxIdFunct get_ctx_id_func)
                : m_p_word_index(_pWordIndex),
                m_get_ctx_id_func(get_ctx_id_func),
                m_chached_ctx(m_context_c_str, MAX_N_GRAM_STRING_LENGTH),
                m_chached_ctx_id(UNDEFINED_WORD_ID) {
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
                 * This method should be called after all the X level grams are read.
                 * @param level the level of the M-grams that were finished to be read
                 */
                void post_Grams(const TModelLevel level){
                    switch( level ) {
                        case MIN_NGRAM_LEVEL:
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
                 * @param ngram the given N-gram vector is expected to have
                 *              exactly N elements (see the template parameters)
                 * @param result the output parameter containing the the result
                 *               probability and possibly some additional meta
                 *               data for the decoder.
                 */
                void queryNGram(const vector<string> & ngram, SProbResult & result);

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
                virtual void post_1_Grams(){};
                
                /**
                 * This method will be called after all the M-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 * @param level the level of the M-grams that were finished to be read
                 */
                virtual void post_M_Grams(const TModelLevel level){};
                
                /**
                 * This method will be called after all the N-grams are read.
                 * The default implementation of this method is present.
                 * If overridden must be called from the child method.
                 */
                virtual void post_N_Grams(){};

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * @param wordId the One-gram id
                 * @return the reference to the storage structure
                 */
                virtual TProbBackOffEntryPair & make_1_GramDataRef(const TShortId wordId) = 0;

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param wordId the One-gram id
                 * @return the reference to the storage structure
                 * @throw out_of_range in case the data can not be located
                 */
                virtual const TProbBackOffEntryPair & get_1_GramDataRef(const TShortId wordId) = 0;

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * @param level the value of M in the M-gram
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @return the reference to the storage structure
                 */
                virtual TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) = 0;

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param level the value of M in the M-gram
                 * @param wordId the id of the M-gram's last word
                 * @param ctxId the M-gram context (the M-gram's prefix) id
                 * @return the reference to the storage structure
                 * @throw out_of_range in case the data can not be located
                 */
                virtual const TProbBackOffEntryPair& get_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) = 0;

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
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * @param wordId the id of the N-gram's last word
                 * @param ctxId the N-gram context (the N-gram's prefix) id
                 * @return the reference to the storage structure
                 * @throw out_of_range in case the data can not be located
                 */
                virtual const TLogProbBackOff& get_N_GramDataRef(const TShortId wordId, const TLongId ctxId) = 0;

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
                    return mGramWordIds[N - 2];
                }

                /**
                 * Gets the word hash for the last word in the N-gram
                 * @return the word hash for the last word in the N-gram
                 */
                inline const TShortId & getNGramEndWordHash() {
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
                inline void tokensToId(const vector<string> & tokens, TShortId wordHashes[N]) {
                    //The start index depends on the value M of the given M-Gram
                    TModelLevel idx = N - tokens.size();
                    LOG_DEBUG1 << "Computing hashes for the words of a " << SSTR(tokens.size()) << "-gram:" << END_LOG;
                    for (vector<string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
                        wordHashes[idx] = m_p_word_index->getId(*it);
                        LOG_DEBUG1 << "wordId('" << *it << "') = " << SSTR(wordHashes[idx]) << END_LOG;
                        idx++;
                    }
                }

                /**
                 * Converts the given tokens to hashes and stores it in mGramWordHashes
                 * @param ngram the n-gram tokens to convert to hashes
                 */
                inline void storeNGramHashes(const vector<string> & ngram) {
                    //First transform the given M-gram into word hashes.
                    tokensToId(ngram, mGramWordIds);
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
                 * @return the computed context id
                 * @throws out_of_range in case the context could not be computed, the context M-gram is not present in the trie
                 */
                inline TLongId getQueryContextId(const TModelLevel ctxLen, bool isBackOff) {
                    const TModelLevel mGramEndIdx = (isBackOff ? (N - 2) : (N - 1));
                    const TModelLevel eIdx = mGramEndIdx;
                    const TModelLevel bIdx = mGramEndIdx - ctxLen;
                    TModelLevel idx = bIdx;

                    LOG_DEBUG3 << "Computing ctxId for context length: " << SSTR(ctxLen)
                            << " for a  " << (isBackOff ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    TLongId ctxId = mGramWordIds[idx];
                    LOG_DEBUG3 << "First word @ idx: " << SSTR(idx) << " has wordId: " << SSTR(ctxId) << END_LOG;
                    idx++;

                    //Compute the subsequent context ids
                    for (; idx < eIdx;) {
                        LOG_DEBUG3 << "Start searching ctxId for mGramWordIds[" << SSTR(idx) << "]: " << SSTR(mGramWordIds[idx]) << " prevCtxId: " << SSTR(ctxId) << END_LOG;
                        ctxId = m_get_ctx_id_func(mGramWordIds[idx], ctxId, (idx - bIdx) + 1);
                        LOG_DEBUG3 << "getContextId(" << SSTR(mGramWordIds[idx]) << ", prevCtxId) = " << SSTR(ctxId) << END_LOG;
                        idx++;
                    }

                    LOG_DEBUG3 << "Resulting context hash for context length " << SSTR(ctxLen)
                            << " of a  " << (isBackOff ? "back-off" : "probability")
                            << " computation is: " << SSTR(ctxId) << END_LOG;

                    return ctxId;
                }

                /**
                 * This function computes the hash context of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must be called on M-grams with M > 1!
                 * 
                 * @param gram the N-gram with its tokens to create context for
                 * @return the resulting hash of the context(w1 w2 w3)
                 */
                template<DebugLevel logLevel>
                inline TLongId getContextId(const SRawNGram & gram) {
                    TLongId ctxId;

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (getCachedContextId(gram, ctxId)) {
                        //Get the start context value for the first token
                        const string & token = gram.tokens[0].str();

                        //There is no id cached for this M-gram context - compute it
                        ctxId = m_p_word_index->getId(token);

                        LOGGER(logLevel) << "ctxId = getId('" << token << "') = " << SSTR(ctxId) << END_LOG;

                        //Iterate and compute the hash:
                        for (int i = 1; i < (gram.level - 1); i++) {
                            const string & token = gram.tokens[i].str();
                            const TShortId wordId = m_p_word_index->getId(token);
                            LOGGER(logLevel) << "wordId = getId('" << token << "') = " << SSTR(wordId) << END_LOG;
                            ctxId = m_get_ctx_id_func(wordId, ctxId, i + 1);
                            LOGGER(logLevel) << "ctxId = createContext( wordId, ctxId ) = " << SSTR(ctxId) << END_LOG;
                        }

                        //Cache the newly computed context id for the given n-gram context
                        setCacheContextId(gram, ctxId);
                    }

                    return ctxId;
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
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id)  << END_LOG;
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

                //The temporary data structure to store the N-gram query word hashes
                TShortId mGramWordIds[N];

                /**
                 * This recursive function implements the computation of the
                 * N-Gram probabilities in the Back-Off Language Model. The
                 * N-Gram hashes are obtained from the _wordHashes member
                 * variable of the class. So it must be pre-set with proper
                 * word hash values first!
                 * @param level the M-gram level for which the probability is to be computed
                 * @return the computed probability value
                 */
                TLogProbBackOff computeLogProbability(const TModelLevel level);

                /**
                 * This recursive function allows to get the back-off weight for the current context.
                 * The N-Gram hashes are obtained from the pre-computed data member array _wordHashes
                 * @param level the M-gram level for which the back-off weight is to be found,
                 * is equal to the context length of the K-Gram in the caller function
                 * @return the resulting back-off weight probability
                 */
                TLogProbBackOff getBackOffWeight(const TModelLevel level);

            };

            //Handy type definitions for the tries of different sizes and with.without caches
            typedef ATrie<MAX_NGRAM_LEVEL> TFiveTrie;
        }
    }
}
#endif	/* ITRIES_HPP */

