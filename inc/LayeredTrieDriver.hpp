/* 
 * File:   LayeredTrieDriver.hpp
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

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"

#include "BitmapHashCache.hpp"

#include "GenericTrieBase.hpp"
#include "LayeredTrieBase.hpp"

#include "G2DHashMapTrie.hpp"
#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"

using namespace std;
using namespace uva::smt::exceptions;
using namespace uva::smt::file;
using namespace uva::smt::hashing;
using namespace uva::smt::tries;
using namespace uva::smt::tries::caching;
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
             * This is the common generic trie base class for layered tries.
             * @param N the maximum level of the considered N-gram, i.e. the N value
             * @param TrieType the type of word index to be used
             */
            template<typename TrieType >
            class LayeredTrieDriver : public GenericTrieBase<TrieType::max_level, typename TrieType::WordIndexType> {
            public:
                typedef GenericTrieBase<TrieType::max_level, typename TrieType::WordIndexType> BASE;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef typename TrieType::TMGramQuery TMGramQuery;
                typedef function<bool (const TrieType&, const TShortId, TLongId &) > TGetCtxIdFunct;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit LayeredTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<TrieType::max_level, WordIndexType>(word_index), m_trie(word_index),
                m_chached_ctx_id(WordIndexType::UNDEFINED_WORD_ID) {

                    //Clear the memory for the buffer and initialize it
                    memset(m_context_c_str, 0, MAX_N_GRAM_STRING_LENGTH * sizeof (char));
                    m_context_c_str[0] = '\0';

                    LOG_DEBUG3 << "Creating the TextPieceReader with a data ptr" << END_LOG;
                    m_chached_ctx.set(m_context_c_str, MAX_N_GRAM_STRING_LENGTH);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline bool is_bitmap_hash_cache() const {
                    return m_trie.is_bitmap_hash_cache();
                }

                /**
                 * @see GenericTrieBase
                 */
                void pre_allocate(const size_t counts[TrieType::max_level]) {
                    //Do the pre-allocation in the trie
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                void add_1_gram(const T_M_Gram &gram);

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel level>
                void add_m_gram(const T_M_Gram &gram);

                /**
                 * @see GenericTrieBase
                 */
                void add_n_gram(const T_M_Gram &gram);

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel curr_level>
                void get_prob_weight(TMGramQuery & query) const;

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel curr_level>
                void add_back_off_weight(TMGramQuery & query) const;

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() const {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                bool is_post_grams(const TModelLevel level) const {
                    return m_trie.is_post_grams(level);
                };

                /**
                 * @see GenericTrieBase
                 */
                inline void post_grams(const TModelLevel level) {
                    m_trie.post_grams(level);
                };

                /**
                 * The basic class destructor
                 */
                virtual ~LayeredTrieDriver() {
                };

            protected:

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                LayeredTrieDriver(const LayeredTrieDriver& orig)
                : GenericTrieBase<TrieType::max_level, WordIndexType>(orig.get_word_index()),
                m_trie(orig.get_word_index()),
                m_chached_ctx(), m_chached_ctx_id(WordIndexType::UNDEFINED_WORD_ID) {
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
                 * @param is_back_off is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @param query the query state object
                 * @param ctx_id [out] the context id to be computed
                 * @return the true if the context could be computed, otherwise false
                 * @throws nothing
                 */
                template<bool is_back_off, TModelLevel curr_level>
                inline bool get_query_context_Id(const TMGramQuery & query, TLongId & ctx_id) const {
                    const TModelLevel mgram_end_idx = (is_back_off ? (TrieType::max_level - 2) : (TrieType::max_level - 1));
                    const TModelLevel end_idx = mgram_end_idx;
                    const TModelLevel begin_idx = mgram_end_idx - (curr_level - 1);
                    TModelLevel idx = begin_idx;

                    LOG_DEBUG1 << "Computing id of the " << SSTR(curr_level)
                            << "-gram " << (is_back_off ? "back-off" : "probability")
                            << " context" << END_LOG;

                    //Compute the first words' hash
                    ctx_id = query.m_query_word_ids[idx];
                    LOG_DEBUG1 << "First word @ idx: " << SSTR(idx) << " has wordId: " << SSTR(ctx_id) << END_LOG;
                    idx++;

                    //Compute the subsequent context ids
                    for (; idx < end_idx;) {
                        LOG_DEBUG1 << "Start searching ctx_id for m_query_word_ids[" << SSTR(idx) << "]: "
                                << SSTR(query.m_query_word_ids[idx]) << " prevCtxId: " << SSTR(ctx_id) << END_LOG;
                        if (get_ctx_id_func[(idx - begin_idx) + 1](m_trie, query.m_query_word_ids[idx], ctx_id )) {
                            LOG_DEBUG1 << "getContextId(" << SSTR(query.m_query_word_ids[idx])
                                    << ", prevCtxId) = " << SSTR(ctx_id) << END_LOG;
                            idx++;
                        } else {
                            //The next context id could not be retrieved
                            return false;
                        }
                    }

                    LOG_DEBUG1 << "Resulting id for the " << SSTR(curr_level)
                            << "-gram " << (is_back_off ? "back-off" : "probability")
                            << " context is: " << SSTR(ctx_id) << END_LOG;

                    return true;
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
                template<DebugLevelsEnum log_level>
                inline bool get_context_id(const T_M_Gram & gram, TLongId &ctxId) {
                    //Try to retrieve the context from the cache, if not present then compute it
                    if (get_cached_context_id(gram, ctxId)) {
                        //Get the start context value for the first token
                        TShortId wordId = m_trie.get_word_index().get_word_id(gram.tokens[0]);

                        //There is no id cached for this M-gram context - find it
                        if (wordId != WordIndexType::UNKNOWN_WORD_ID) {
                            //The first word id is the first context id
                            ctxId = wordId;
                            LOGGER(log_level) << "ctxId = getId('" << gram.tokens[0].str()
                                    << "') = " << SSTR(ctxId) << END_LOG;

                            //Iterate and compute the hash:
                            for (int i = 1; i < (gram.level - 1); i++) {
                                wordId = m_trie.get_word_index().get_word_id(gram.tokens[i]);
                                if (wordId != WordIndexType::UNKNOWN_WORD_ID) {
                                    LOGGER(log_level) << "wordId = getId('" << gram.tokens[i].str()
                                            << "') = " << SSTR(wordId) << END_LOG;
                                    if (get_ctx_id_func[i + 1](m_trie, wordId, ctxId)) {
                                        LOGGER(log_level) << "ctxId = computeCtxId( "
                                                << "wordId, ctxId ) = " << SSTR(ctxId) << END_LOG;
                                    } else {
                                        //The next context id could not be computed
                                        LOGGER(log_level) << "The next context for wordId: "
                                                << SSTR(wordId) << " and ctxId: "
                                                << SSTR(ctxId) << "on level: " << SSTR((i + 1))
                                                << "could not be computed!" << END_LOG;
                                        return false;
                                    }
                                } else {
                                    //The next word Id was not found, it is
                                    //unknown, so we can stop searching
                                    LOGGER(log_level) << "The wordId for '" << gram.tokens[i].str()
                                            << "' could not be found!" << END_LOG;
                                    return false;
                                }
                            }

                            //Cache the newly computed context id for the given n-gram context
                            set_cache_context_id(gram, ctxId);

                            //The context Id was found in the Trie
                            LOGGER(log_level) << "The ctxId could be computed, "
                                    << "it's value is: " << SSTR(ctxId) << END_LOG;
                            return true;
                        } else {
                            //The context id could not be computed as
                            //the first N-gram's word is already unknown
                            LOGGER(log_level) << "The wordId for '" << gram.tokens[0].str()
                                    << "' could not be found!" << END_LOG;
                            return false;
                        }
                    } else {
                        //The context Id was found in the cache
                        LOGGER(log_level) << "The ctxId was found in cache, "
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
                inline bool get_cached_context_id(const T_M_Gram &mGram, TLongId & result) const {
                    if (m_chached_ctx == mGram.context) {
                        result = m_chached_ctx_id;
                        LOG_DEBUG2 << "Cache MATCH! [" << m_chached_ctx << "] == [" << mGram.context
                                << "], for m-gram: " << tokensToString<TrieType::max_level>(mGram)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return false;
                    } else {
                        LOG_DEBUG2 << "Cache MISS! [" << m_chached_ctx << "] != [" << mGram.context
                                << "], for m-gram: " << tokensToString<TrieType::max_level>(mGram)
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return true;
                    }
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void set_cache_context_id(const T_M_Gram &mGram, TLongId & stx_id) {
                    LOG_DEBUG2 << "Caching context = [ " << mGram.context << " ], id = " << stx_id
                            << ", for m-gram: " << tokensToString<TrieType::max_level>(mGram) << END_LOG;

                    m_chached_ctx.copy_string<MAX_N_GRAM_STRING_LENGTH>(mGram.context);
                    m_chached_ctx_id = stx_id;

                    LOG_DEBUG2 << "Cached context = [ " << m_chached_ctx
                            << " ], id = " << SSTR(m_chached_ctx_id) << END_LOG;
                }

            private:
                //Stores the trie
                TrieType m_trie;

                //The actual storage for the cached context c string
                char m_context_c_str[MAX_N_GRAM_STRING_LENGTH];
                //Stores the cached M-gram context (for 1 < M <= N )
                TextPieceReader m_chached_ctx;
                //Stores the cached M-gram context value (for 1 < M <= N )
                TLongId m_chached_ctx_id;

                //Stores the pointers to instances of th get_ctx_id function templates
                static const TGetCtxIdFunct get_ctx_id_func[];
            };

            template<typename TrieType>
            const typename LayeredTrieDriver<TrieType>::TGetCtxIdFunct LayeredTrieDriver<TrieType>::get_ctx_id_func[] = {
                NULL,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_1>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_2>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_3>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_4>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_5>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_6>,
                &TrieType::template get_ctx_id<M_GRAM_LEVEL_7>
            };

#define TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, TYPE) \
            typedef LayeredTrieDriver< T##TRIE_NAME##TYPE > TLayeredTrieDriver##TRIE_NAME##TYPE;

#define TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(TRIE_NAME) \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Basic); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Count); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptBasic); \
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptCount);

            /**************************************************************************/
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2DMapTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2WArrayTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(W2CArrayTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(W2CHybridTrie);
            TYPEDEF_LAYERED_DRIVER_TEMPLATES_NAME(C2DHybridTrie);
            /**************************************************************************/
        }
    }
}
#endif	/* ITRIES_HPP */

