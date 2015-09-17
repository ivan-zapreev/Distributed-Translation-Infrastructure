/* 
 * File:   ALayeredTrie.hpp
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

#ifndef ALAYEREDTRIE_HPP
#define	ALAYEREDTRIE_HPP

#include <string>       // std::string
#include <functional>   // std::function 

#include "ATrie.hpp"
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
            LOG_WARNING << "The " << gram.level << "-Gram : " << tokensToString<N>(gram)                     \
                        << " has been already seen! Word Id: " << SSTR(wordHash)                             \
                        << ", context Id: " << SSTR(contextId) << ". "                                       \
                        << "Changing the (prob,back-off) data from ("                                        \
                        << prevProb << "," << prevBackOff << ") to ("                                        \
                        << newProb << "," << newBackOff << ")" << END_LOG;

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
             * This is a common abstract class for a Layered Trie implementation. Layered
             * means we need to go from context + word to context to get the N-Gram id.
             * The purpose of having this as a template class is performance optimization.
             * @param N - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel N >
            class ALayeredTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 * @param is_birmap_hash_cache - allows to enable the bitmap hash cache for the M-grams.
                 * the latter records the hashes of all the M-gram present in the tries and then before
                 * querying checks if the hash of the queries M-gram is present if not then we do an
                 * immediate back-off, otherwise we search for the M-gram. This is an experimental feature
                 * therefore it the parameter's default is false. Also this feature is to be used with
                 * the multi-level context index tries which require a lot of searching when looking
                 * for an M-gram.
                 */
                explicit ALayeredTrie(AWordIndex * const _pWordIndex,
                        TGetCtxIdFunct get_ctx_id_func, bool is_birmap_hash_cache)
                : ATrie<N>(_pWordIndex, is_birmap_hash_cache),
                m_get_ctx_id_func(get_ctx_id_func),
                m_chached_ctx_id(AWordIndex::UNDEFINED_WORD_ID) {
                    //Clear the memory for the buffer and initialize it
                    memset(m_context_c_str, 0, MAX_N_GRAM_STRING_LENGTH * sizeof (char));
                    m_context_c_str[0] = '\0';

                    LOG_DEBUG3 << "Creating the TextPieceReader with a data ptr" << END_LOG;
                    m_chached_ctx.set(m_context_c_str, MAX_N_GRAM_STRING_LENGTH);

                    //This one is needed for having a proper non-null word index pointer.
                    if (_pWordIndex == NULL) {
                        stringstream msg;
                        msg << "Unable to use " << __FILE__ << ", the word index pointer must not be NULL!";
                        throw Exception(msg.str());
                    }
                };

                /**
                 * This method adds a 1-Gram (word) to the trie.
                 * It it snot guaranteed that the parameter will be checked to be a 1-Gram!
                 * @param gram the 1-Gram data
                 */
                virtual void add_1_gram(const T_M_Gram &gram);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param gram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                virtual void add_m_gram(const T_M_Gram &gram);

                /**
                 * This method adds a N-Gram (word) to the trie where
                 * It it snot guaranteed that the parameter will be checked to be a N-Gram!
                 * @param gram the N-Gram data
                 */
                virtual void add_n_gram(const T_M_Gram &gram);

                /**
                 * The basic class destructor
                 */
                virtual ~ALayeredTrie() {
                };

            protected:

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
                 * This function allows to retrieve the probability stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * @see ATrie
                 */
                virtual bool add_prob_weight(const TModelLevel level, TLogProbBackOff & prob);

                /**
                 * This function allows to retrieve the back-off stored for the given M-gram level.
                 * If the value is found then it must be added to the prob parameter of the function.
                 * If the value is not found then the prob parameter of the function must not be changed.
                 * In that case the back-off weight is just zero.
                 * @see ATrie
                 */
                virtual void add_back_off_weight(const TModelLevel level, TLogProbBackOff & prob);

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                ALayeredTrie(const ALayeredTrie& orig)
                : ATrie<N>(NULL),
                m_get_ctx_id_func(NULL),
                m_chached_ctx(),
                m_chached_ctx_id(AWordIndex::UNDEFINED_WORD_ID) {
                    throw Exception("ATrie copy constructor is not to be used, unless implemented!");
                };

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
                 * @param is_back_off is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @param ctxId [out] the context id to be computed
                 * @return the true if the context could be computed, otherwise false
                 * @throws nothing
                 */
                template<bool is_back_off>
                inline bool get_query_context_Id(const TModelLevel ctxLen, TLongId & ctxId) {
                    const TModelLevel mGramEndIdx = (is_back_off ? (N - 2) : (N - 1));
                    const TModelLevel eIdx = mGramEndIdx;
                    const TModelLevel bIdx = mGramEndIdx - ctxLen;
                    TModelLevel idx = bIdx;

                    LOG_DEBUG1 << "Computing ctxId for context length: " << SSTR(ctxLen)
                            << " for a  " << (is_back_off ? "back-off" : "probability")
                            << " computation" << END_LOG;

                    //Compute the first words' hash
                    ctxId = ATrie<N>::m_tmp_word_ids[idx];
                    LOG_DEBUG1 << "First word @ idx: " << SSTR(idx) << " has wordId: " << SSTR(ctxId) << END_LOG;
                    idx++;

                    //Since the first word defines the second word context, and
                    //if this word is unknown then there is definitely no data
                    //for this N-gram in the trie ... so we through!
                    if (ctxId < AWordIndex::MIN_KNOWN_WORD_ID) {
                        LOG_DEBUG1 << "The first " << SSTR(ctxLen + 1) << " wordId == "
                                << SSTR(ctxId) << " i.e. <unk>, need to back-off!" << END_LOG;
                        return false;
                    } else {
                        //Compute the subsequent context ids
                        for (; idx < eIdx;) {
                            LOG_DEBUG1 << "Start searching ctxId for mGramWordIds[" << SSTR(idx) << "]: "
                                    << SSTR(ATrie<N>::m_tmp_word_ids[idx]) << " prevCtxId: " << SSTR(ctxId) << END_LOG;
                            if (m_get_ctx_id_func(ATrie<N>::m_tmp_word_ids[idx], ctxId, (idx - bIdx) + 1)) {
                                LOG_DEBUG1 << "getContextId(" << SSTR(ATrie<N>::m_tmp_word_ids[idx])
                                        << ", prevCtxId) = " << SSTR(ctxId) << END_LOG;
                                idx++;
                            } else {
                                //The next context id could not be retrieved
                                return false;
                            }
                        }

                        LOG_DEBUG1 << "Resulting context hash for context length " << SSTR(ctxLen)
                                << " of a  " << (is_back_off ? "back-off" : "probability")
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
                inline bool getContextId(const T_M_Gram & gram, TLongId &ctxId) {
                    //Try to retrieve the context from the cache, if not present then compute it
                    if (getCachedContextId(gram, ctxId)) {
                        //Get the start context value for the first token
                        TShortId wordId;

                        //There is no id cached for this M-gram context - find it
                        if (ATrie<N>::get_word_index()->get_word_id(gram.tokens[0], wordId)) {
                            //The first word id is the first context id
                            ctxId = wordId;
                            LOGGER(logLevel) << "ctxId = getId('" << gram.tokens[0].str()
                                    << "') = " << SSTR(ctxId) << END_LOG;

                            //Iterate and compute the hash:
                            for (int i = 1; i < (gram.level - 1); i++) {
                                if (ATrie<N>::get_word_index()->get_word_id(gram.tokens[i], wordId)) {
                                    LOGGER(logLevel) << "wordId = getId('" << gram.tokens[i].str()
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
                                    LOGGER(logLevel) << "The wordId for '" << gram.tokens[i].str()
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
                            LOGGER(logLevel) << "The wordId for '" << gram.tokens[0].str()
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
                inline bool getCachedContextId(const T_M_Gram &mGram, TLongId & result) {
                    if (m_chached_ctx == mGram.context) {
                        result = m_chached_ctx_id;
                        LOG_DEBUG2 << "Cache MATCH! [" << m_chached_ctx << "] == [" << mGram.context
                                << "], for m-gram: " << tokensToString<N>(mGram)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return false;
                    } else {
                        LOG_DEBUG2 << "Cache MISS! [" << m_chached_ctx << "] != [" << mGram.context
                                << "], for m-gram: " << tokensToString<N>(mGram)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return true;
                    }
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void setCacheContextId(const T_M_Gram &mGram, TLongId & stx_id) {
                    LOG_DEBUG2 << "Caching context = [ " << mGram.context << " ], id = " << stx_id
                            << ", for m-gram: " << tokensToString<N>(mGram) << END_LOG;

                    m_chached_ctx.copy_string<MAX_N_GRAM_STRING_LENGTH>(mGram.context);
                    m_chached_ctx_id = stx_id;

                    LOG_DEBUG2 << "Cached context = [ " << m_chached_ctx
                            << " ], id = " << SSTR(m_chached_ctx_id) << END_LOG;
                }

            private:

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

            };
        }
    }
}
#endif	/* ITRIES_HPP */

