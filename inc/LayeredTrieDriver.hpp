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

#include "GenericTrieBase.hpp"

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
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            //This macro is needed to report the collision detection warnings!
#define REPORT_COLLISION_WARNING(gram, wordHash, contextId, prevProb, prevBackOff, newProb, newBackOff)   \
            LOG_WARNING << "The " << gram.get_m_gram_level() << "-Gram : " << (string) gram               \
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
            class LayeredTrieDriver : public GenericTrieBase<TrieType::MAX_LEVEL, typename TrieType::WordIndexType> {
            public:
                static constexpr TModelLevel MAX_LEVEL = TrieType::MAX_LEVEL;
                typedef typename TrieType::WordIndexType WordIndexType;
                typedef typename WordIndexType::TWordIdType TWordIdType;
                typedef GenericTrieBase<MAX_LEVEL, WordIndexType> BASE;
                //This is the function pointer type for the function that computes the m-gram context id
                typedef function<bool (const TrieType&, const TShortId, TLongId &) > TGetCtxIdFunct;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit LayeredTrieDriver(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType>(word_index), m_trie(word_index),
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
                constexpr static inline bool needs_bitmap_hash_cache() {
                    return TrieType::needs_bitmap_hash_cache();
                }

                /**
                 * @see GenericTrieBase
                 */
                inline void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    //Do the pre-allocation in the trie
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    //Define the context id variabe
                    TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                    //Get the word id of this unigram, so there is just one word in it and its the end one
                    const TShortId end_word_id = gram.get_end_word_id();

                    //Obtain the context id for non-unigram case
                    if (CURR_LEVEL != M_GRAM_LEVEL_1) {
                        get_context_id<CURR_LEVEL, DebugLevelsEnum::DEBUG2>(gram, ctx_id);
                    }

                    //Add the m-gram payload
                    m_trie.template add_m_gram_payload<CURR_LEVEL>(end_word_id, ctx_id, gram.m_payload);
                }

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
                GPR_Enum get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const;

                /**
                 * @see GenericTrieBase
                 */
                inline void log_trie_type_usage_info() const {
                    m_trie.log_trie_type_usage_info();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                bool is_post_grams() const {
                    return m_trie.template is_post_grams<CURR_LEVEL>();
                };

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    m_trie.template post_grams<CURR_LEVEL>();
                };

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    m_trie.get_unk_word_payload(payload);
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
                LayeredTrieDriver(const LayeredTrieDriver & orig)
                : GenericTrieBase<MAX_LEVEL, WordIndexType>(orig.get_word_index()),
                m_trie(orig.get_word_index()),
                m_chached_ctx(), m_chached_ctx_id(WordIndexType::UNDEFINED_WORD_ID) {
                    throw Exception("ATrie copy constructor is not to be used, unless implemented!");
                };

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
                template<TModelLevel CURR_LEVEL, bool GET_BACK_OFF_CTX_ID, DebugLevelsEnum LOG_LEVEL = DebugLevelsEnum::DEBUG1>
                inline TModelLevel search_m_gram_ctx_id(const TWordIdType * const word_ids, TLongId & prev_ctx_id, TLongId & ctx_id) const {
                    switch (CURR_LEVEL) {
                        case M_GRAM_LEVEL_2:
                            ctx_id = word_ids[0];
                            return M_GRAM_LEVEL_2;
                        case M_GRAM_LEVEL_3:
                            if (GET_BACK_OFF_CTX_ID) prev_ctx_id = word_ids[0];
                            ctx_id = word_ids[0];
                            if (m_trie. template get_ctx_id<M_GRAM_LEVEL_2>(word_ids[1], ctx_id)) {
                                return M_GRAM_LEVEL_3;
                            } else {
                                return M_GRAM_LEVEL_2;
                            }
                        case M_GRAM_LEVEL_4:
                            ctx_id = word_ids[0];
                            if (m_trie. template get_ctx_id<M_GRAM_LEVEL_2>(word_ids[1], ctx_id)) {
                                if (GET_BACK_OFF_CTX_ID) prev_ctx_id = ctx_id;
                                if (m_trie. template get_ctx_id<M_GRAM_LEVEL_3>(word_ids[2], ctx_id)) {
                                    return M_GRAM_LEVEL_4;
                                } else {
                                    return M_GRAM_LEVEL_3;
                                }
                            } else {
                                return M_GRAM_LEVEL_2;
                            }
                        case M_GRAM_LEVEL_5:
                            ctx_id = word_ids[0];
                            if (m_trie. template get_ctx_id<M_GRAM_LEVEL_2>(word_ids[1], ctx_id)) {
                                if (m_trie. template get_ctx_id<M_GRAM_LEVEL_3>(word_ids[2], ctx_id)) {
                                    if (GET_BACK_OFF_CTX_ID) prev_ctx_id = ctx_id;
                                    if (m_trie. template get_ctx_id<M_GRAM_LEVEL_4>(word_ids[3], ctx_id)) {
                                        return M_GRAM_LEVEL_5;
                                    } else {
                                        return M_GRAM_LEVEL_4;
                                    }
                                } else {
                                    return M_GRAM_LEVEL_3;
                                }
                            } else {
                                return M_GRAM_LEVEL_2;
                            }
                        default:
                            THROW_EXCEPTION(string("The sub-m-gram level is not supported, CURR_LEVEL: ").append(std::to_string(CURR_LEVEL)));
                    }
                }

                /**
                 * This function computes the context id of the N-gram given by the tokens, e.g. [w1 w2 w3 w4]
                 * 
                 * WARNING: Must be called on M-grams with M > 1!
                 * 
                 * @param gram the m-gram we need to compute the contex tfor. 
                 * @param mgram_word_ids the m-gram word ids aligned to the end of the array
                 * @param the resulting hash of the context(w1 w2 w3)
                 * @return true if the context was found otherwise false
                 */
                template<TModelLevel CURR_LEVEL, DebugLevelsEnum LOG_LEVEL>
                inline void get_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId & ctx_id) {
                    //Perform sanity check for the level values they should be the same!
                    if (DO_SANITY_CHECKS && (CURR_LEVEL != gram.get_m_gram_level())) {
                        stringstream msg;
                        msg << "The improper level values! Template level parameter = " << SSTR(CURR_LEVEL)
                                << " but the m-gram level value is: " << SSTR(gram.get_m_gram_level());
                        throw Exception(msg.str());
                    }

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (get_cached_context_id(gram, ctx_id)) {
                        //Compute the context id, check on the level
                        const TModelLevel ctx_level = search_m_gram_ctx_id<CURR_LEVEL, false, LOG_LEVEL>(gram.first_word_id(), ctx_id, ctx_id);

                        //Do sanity check if needed
                        if (DO_SANITY_CHECKS && ((CURR_LEVEL - 1) != ctx_level)) {
                            //The next context id could not be computed
                            stringstream msg;
                            msg << "The m-gram:" << (string) gram << " context could not be computed!";
                            throw Exception(msg.str());
                        }

                        //Cache the newly computed context id for the given n-gram context
                        set_cache_context_id(gram, ctx_id);

                        //The context Id was found in the Trie
                        LOGGER(LOG_LEVEL) << "The ctx_id could be computed, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    } else {
                        //The context Id was found in the cache
                        LOGGER(LOG_LEVEL) << "The ctx_id was found in cache, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    }
                }

                /**
                 * Allows to retrieve the cached context id for the given M-gram if any
                 * @param mGram the m-gram to get the context id for
                 * @param result the output parameter, will store the cached id, if any
                 * @return true if there was nothing cached, otherwise false
                 */
                inline bool get_cached_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId & result) const {
                    if (m_chached_ctx == gram.m_context) {
                        result = m_chached_ctx_id;
                        LOG_DEBUG2 << "Cache MATCH! [" << m_chached_ctx << "] == [" << gram.m_context
                                << "], for m-gram: " << (string) gram
                                << ", cached ctx_id: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return false;
                    } else {
                        LOG_DEBUG2 << "Cache MISS! [" << m_chached_ctx << "] != [" << gram.m_context
                                << "], for m-gram: " << (string) gram
                                << ", cached ctx_id: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return true;
                    }
                }

                /**
                 * Allows to cache the context id of the given m-grams context
                 * @param mGram
                 * @param result
                 */
                inline void set_cache_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId & stx_id) {
                    LOG_DEBUG2 << "Caching context = [ " << gram.m_context << " ], id = " << stx_id
                            << ", for m-gram: " << (string) gram << END_LOG;

                    m_chached_ctx.copy_string<MAX_N_GRAM_STRING_LENGTH>(gram.m_context);
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
            };

            template<typename TrieType>
            constexpr TModelLevel LayeredTrieDriver<TrieType>::MAX_LEVEL;

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

