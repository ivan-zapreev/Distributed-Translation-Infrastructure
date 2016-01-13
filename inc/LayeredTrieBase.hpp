/* 
 * File:   LayeredTrieBase.hpp
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
 * Created on September 20, 2015, 5:39 PM
 */

#ifndef LAYEREDTRIEBASE_HPP
#define	LAYEREDTRIEBASE_HPP


#include <string>       // std::string
#include <cstring>      // std::memcmp std::memcpy

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __LayeredTrieBase {

                /**
                 * Allows to obtain the context and previous context id for the sub-m-gram defined by the given template parameters.
                 * @param CURR_LEVEL the level of the sub-m-gram for which the context id is to be computed
                 * @param DO_PREV_CONTEXT true if the previous context id is to be computed, otherwise false
                 * @param LOG_LEVEL the desired debug level
                 * @param word_ids the array of word ids to consider for computing the context id
                 * @param prev_ctx_id the computed previous context id, if computed
                 * @param ctx_id the context id, if computed
                 * @return the level of the m-gram for which the last context id could be computed
                 */
                template<typename TrieType, TModelLevel CURR_LEVEL, bool GET_BACK_OFF_CTX_ID, DebugLevelsEnum LOG_LEVEL = DebugLevelsEnum::DEBUG1>
                inline TModelLevel search_m_gram_ctx_id(const TrieType & trie, const typename TrieType::WordIndexType::TWordIdType * const word_ids, TLongId & prev_ctx_id, TLongId & ctx_id) {
                    //Assert that this method is called for proper m-gram levels
                    ASSERT_SANITY_THROW(((CURR_LEVEL < M_GRAM_LEVEL_2) || (CURR_LEVEL > M_GRAM_LEVEL_5)), string("The level: ") + std::to_string(CURR_LEVEL) + string(" is not supported yet!"));

                    //The initial context for anything larger than a unigram is the first word id
                    ctx_id = word_ids[0];

                    //If we are at least at a level 3 m-gram
                    if (CURR_LEVEL >= M_GRAM_LEVEL_3) {
                        if (GET_BACK_OFF_CTX_ID && (CURR_LEVEL == M_GRAM_LEVEL_3)) prev_ctx_id = ctx_id;
                        if (trie.get_ctx_id(M_GRAM_LEVEL_2 - M_GRAM_LEVEL_2, word_ids[1], ctx_id)) {
                            //If we are at least at a level 4 m-gram
                            if (CURR_LEVEL >= M_GRAM_LEVEL_4) {
                                if (GET_BACK_OFF_CTX_ID && (CURR_LEVEL == M_GRAM_LEVEL_4)) prev_ctx_id = ctx_id;
                                if (trie.get_ctx_id(M_GRAM_LEVEL_3 - M_GRAM_LEVEL_2, word_ids[2], ctx_id)) {
                                    //If we are at least at a level 5 m-gram
                                    if (CURR_LEVEL >= M_GRAM_LEVEL_5) {
                                        if (GET_BACK_OFF_CTX_ID && (CURR_LEVEL == M_GRAM_LEVEL_5)) prev_ctx_id = ctx_id;
                                        if (trie.get_ctx_id(M_GRAM_LEVEL_4 - M_GRAM_LEVEL_2, word_ids[3], ctx_id)) {
                                            return M_GRAM_LEVEL_5;
                                        }
                                    }
                                    return M_GRAM_LEVEL_4;
                                }
                            }
                            return M_GRAM_LEVEL_3;
                        }
                    }
                    return M_GRAM_LEVEL_2;
                }

                /**
                 * This function computes the context id of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must be called on M-grams with M > 1!
                 * 
                 * @param gram the m-gram we need to compute the context for. 
                 * @param mgram_word_ids the m-gram word ids aligned to the end of the array
                 * @param the resulting hash of the context(w1 w2 w3)
                 * @return true if the context was found otherwise false
                 */
                template<typename TrieType, TModelLevel CURR_LEVEL, DebugLevelsEnum LOG_LEVEL>
                inline void get_context_id(TrieType & trie, const T_Model_M_Gram<typename TrieType::WordIndexType> &gram, TLongId & ctx_id) {
                    //Perform sanity check for the level values they should be the same!
                    ASSERT_SANITY_THROW(CURR_LEVEL != gram.get_m_gram_level(),
                            string("The improper level values! Template level parameter = ") + std::to_string(CURR_LEVEL) +
                            string(" but the m-gram level value is: ") + std::to_string(gram.get_m_gram_level()));

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (trie.template get_cached_context_id<CURR_LEVEL>(gram, ctx_id)) {
                        //Compute the context id, check on the level
                        const TModelLevel ctx_level = search_m_gram_ctx_id<TrieType, CURR_LEVEL, false, LOG_LEVEL>(trie, gram.word_ids(), ctx_id, ctx_id);

                        LOG_DEBUG4 << "The context level is: " << SSTR(ctx_level) << ", the current level is " << SSTR(CURR_LEVEL) << END_LOG;

                        //Do sanity check if needed
                        ASSERT_SANITY_THROW(CURR_LEVEL != ctx_level, string("The m-gram:") +
                                ((string) gram) + string(" context could not be computed!"));

                        //Cache the newly computed context id for the given n-gram context
                        trie.template set_cache_context_id<CURR_LEVEL>(gram, ctx_id);

                        //The context Id was found in the Trie
                        LOGGER(LOG_LEVEL) << "The ctx_id could be computed, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    } else {
                        //The context Id was found in the cache
                        LOGGER(LOG_LEVEL) << "The ctx_id was found in cache, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    }
                }
            }

#define LAYERED_BASE_ENSURE_CONTEXT(query, status) \
            if (query.m_last_ctx_ids[query.m_begin_word_idx] == WordIndexType::UNDEFINED_WORD_ID) { \
                BASE::ensure_context(query, status); \
            } else { \
                status = MGramStatusEnum::GOOD_PRESENT_MGS; \
            }

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<typename TrieType, TModelLevel MAX_LEVEL, typename WordIndexType, uint8_t BITMAP_HASH_CACHE_BUCKETS_FACTOR>
            class LayeredTrieBase : public GenericTrieBase<TrieType, MAX_LEVEL, WordIndexType, BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
            public:
                //Typedef the base class
                typedef GenericTrieBase<TrieType, MAX_LEVEL, WordIndexType, BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit LayeredTrieBase(WordIndexType & word_index)
                : GenericTrieBase<TrieType, MAX_LEVEL, WordIndexType, BITMAP_HASH_CACHE_BUCKETS_FACTOR> (word_index),
                m_zero_payload(ZERO_PROB_WEIGHT, ZERO_BACK_OFF_WEIGHT) {
                    //Clean the cache memory
                    memset(m_cached_ctx, 0, MAX_LEVEL * sizeof (TContextCacheEntry));
                }

                /**
                 * Allows to indicate whether the context id of an m-gram is to be computed while retrieving payloads
                 * @return returns true, by default all layered tries need context ids when searching for data
                 */
                static constexpr bool is_need_getting_ctx_ids() {
                    return true;
                }

                /**
                 * @see GenericTrieBase
                 */
                inline void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    BASE::pre_allocate(counts);
                }

                /**
                 * Allows to get the the new context id for the word and previous context id given the level
                 * @param level_idx the m-gram level index, where m is > 1 and index is computed as m - 2;
                 * @param word_id the word id on this level
                 * @param ctx_id the previous level context id
                 * @return true if computation of the next context is succeeded
                 */
                inline bool get_ctx_id(const TModelLevel level_idx, const TShortId word_id, TLongId & ctx_id) const {
                    THROW_MUST_OVERRIDE();
                }

                /**
                 * Allows to retrieve the cached context id for the given M-gram if any
                 * @param mGram the m-gram to get the context id for
                 * @param result the output parameter, will store the cached id, if any
                 * @return true if there was nothing cached, otherwise false
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_cached_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId & result) const {
                    //Compute the context level
                    constexpr TModelLevel CONTEXT_LEVEL = CURR_LEVEL - 1;
                    //Check if this is the same m-gram
                    if (memcmp(m_cached_ctx[CONTEXT_LEVEL].m_word_ids, gram.word_ids(), CONTEXT_LEVEL * sizeof (TShortId)) == 0) {
                        result = m_cached_ctx[CONTEXT_LEVEL].m_ctx_id;
                        //There was something cached, so no need to search further!
                        return false;
                    }
                    //There was nothing cached, so return keep searching!
                    return true;
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param gram the m-gram to cache
                 * @param ctx_id the m-gram context id to cache.
                 */
                template<TModelLevel CURR_LEVEL>
                inline void set_cache_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId & ctx_id) {
                    //Compute the context level
                    constexpr TModelLevel CONTEXT_LEVEL = CURR_LEVEL - 1;
                    //Copy the context word ids
                    memcpy(m_cached_ctx[CONTEXT_LEVEL].m_word_ids, gram.word_ids(), CONTEXT_LEVEL * sizeof (TShortId));
                    //Store the cache value
                    m_cached_ctx[CONTEXT_LEVEL].m_ctx_id = ctx_id;
                }

            protected:

                /**
                 * For the given query tries to ensure that the context is computed and stored.
                 * Also for the context the payload is retrieved. If the back-off is also not
                 * found sets its payload pointer to the zero payload structure.
                 * WARNING: This method is to be only called for minimal bi-gram queries!
                 * WARNING: Is only to be called if the context has not been computed yet
                 * @param query the query to work with
                 * @return true if the context was successfully computed, otherwise false.
                 */
                inline void ensure_context(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //Get the context id reference for convenience
                    TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];

                    LOG_DEBUG << "Ensuring context or sub-m-gram : [" << SSTR(query.m_begin_word_idx)
                            << ", " << SSTR(query.m_end_word_idx) << "], the last computed context value is: "
                            << ctx_id << END_LOG;

                    //The first context is the first word id
                    ctx_id = query.m_gram[query.m_begin_word_idx];

                    LOG_DEBUG << "Setting the first context value to the first word id: " << ctx_id << END_LOG;

                    //Decrement the end word index to get down to the back-off level
                    query.m_end_word_idx--;

                    //Set the result status to true
                    status = MGramStatusEnum::GOOD_PRESENT_MGS;

                    //Check if the back-off m-gram is a unigram or not
                    if (query.m_begin_word_idx == query.m_end_word_idx) {
                        //The the back-off sub-m-gram is a uni-gram obtain its payload
                        static_cast<const TrieType*> (this)->get_unigram_payload(query);
                    } else {
                        //If the back-off sub-m-gram is not a uni-gram then do the context
                        for (TModelLevel word_idx = query.m_begin_word_idx + 1; word_idx < query.m_end_word_idx; ++word_idx) {
                            LOG_DEBUG2 << "Getting the context id for sub-m-gram: [" << SSTR(query.m_begin_word_idx) << ", " << SSTR(word_idx) << "]" << END_LOG;
                            const TModelLevel & level_idx = CURR_LEVEL_MIN_2_MAP[query.m_begin_word_idx][word_idx];
                            if (!static_cast<const TrieType*> (this)->get_ctx_id(level_idx, query.m_gram[word_idx], ctx_id)) {
                                //If the next context could not be computed, we stop with a bad status
                                status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                break;
                            }
                        }

                        //If the context of the back-off sub-m-gram could be computed then retrieve its payload
                        if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                            //The back-off sub-m-gram is at least a bi-gram and never an n-gram
                            static_cast<const TrieType*> (this)->get_m_gram_payload(query, status);
                        }

                        //If the back-off payload could not be computed, set the zero payload
                        if (status != MGramStatusEnum::GOOD_PRESENT_MGS) {
                            //Set the back-off payload to zero payload!

                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &m_zero_payload;
                        }
                    }
                    //Increment the end word index to get back to the original sub-m-gram
                    query.m_end_word_idx++;

                    LOG_DEBUG << "Ensuring context or sub-m-gram : [" << SSTR(query.m_begin_word_idx)
                            << ", " << SSTR(query.m_end_word_idx) << "] status is: "
                            << status_to_string(status) << END_LOG;
                }

            private:

                //Stores the zero payload for begin used when no payload is found
                const T_M_Gram_Payload m_zero_payload;

                /**
                 * This structure is to store the cached word ids and context ids
                 * @param m_word_ids the word ids identifier of the m-gram 
                 * @param m_ctx_id the cached context id for the m-gram
                 */
                typedef struct {
                    TShortId m_word_ids[MAX_LEVEL];
                    TLongId m_ctx_id;
                } TContextCacheEntry;

                //Stores the cached contexts data 
                TContextCacheEntry m_cached_ctx[MAX_LEVEL];
            };

            //Define the template for instantiating the layered trie class children templates
#define INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(CLASS_NAME, WORD_IDX_TYPE) \
            template class CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >;

        }
    }
}

#endif	/* LAYEREDTRIEBASE_HPP */

