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
                void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    //Do the pre-allocation in the trie
                    m_trie.pre_allocate(counts);
                };

                /**
                 * @see GenericTrieBase
                 */
                void add_1_gram(const T_Model_M_Gram<WordIndexType> &gram);

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram);

                /**
                 * @see GenericTrieBase
                 */
                void add_n_gram(const T_Model_M_Gram<WordIndexType> & gram);

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX>
                bool get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload) const;

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                void get_prob_weight(const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const;

                /**
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                void add_back_off_weight(const T_M_Gram<WordIndexType> & gram, TLogProbBackOff & total_prob) const;

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
                LayeredTrieDriver(const LayeredTrieDriver& orig)
                : GenericTrieBase<MAX_LEVEL, WordIndexType>(orig.get_word_index()),
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
                 * @param IS_BACK_OFF is the boolean flag that determines whether
                 *                  we compute the context for the entire M-Gram
                 *                  or for the back-off sub-M-gram. For the latter
                 *                  we consider w1 w2 w3 w4 only
                 * @param CURR_LEVEL the current level of the m-gram that we are considering
                 * @param LOG_LEVEL the debug level to be used in this function, the default is DebugLevelsEnum::DEBUG1
                 * @param gram the m-gram to compute the context id for
                 * @param ctx_id [out] the context id to be computed
                 * @return the true if the context could be computed, otherwise false
                 * @throws nothing
                 */
                template<DebugLevelsEnum LOG_LEVEL = DebugLevelsEnum::DEBUG1>
                inline bool get_m_gram_ctx_id(const TWordIdType * const begin_wid_iter,
                        const TWordIdType * const end_wid_iter, TLongId & ctx_id) const {
                    //Initialize the current word iterator
                    const TWordIdType * curr_wid_iter = begin_wid_iter;

                    LOGGER(LOG_LEVEL) << "Computing id of the m-gram context" << END_LOG;

                    //Compute the first words' hash
                    ctx_id = *curr_wid_iter;
                    LOGGER(LOG_LEVEL) << "m_word_ids[0]: " << SSTR(ctx_id) << END_LOG;
                    curr_wid_iter++;

                    //The word has to be known, otherwise it is an error situation
                    if (DO_SANITY_CHECKS && (ctx_id == WordIndexType::UNKNOWN_WORD_ID)) {
                        stringstream msg;
                        msg << "The first word of the m-gram is unknown!";
                        throw Exception(msg.str());
                    }

                    //Compute the subsequent context ids
                    TModelLevel idx = 1; //This is the "second word index"
                    for (; curr_wid_iter != end_wid_iter;) {
                        LOGGER(LOG_LEVEL) << "Start searching ctx_id for m_word_ids[" << SSTR(idx) << "]: "
                                << SSTR(*curr_wid_iter) << " prevCtxId: " << SSTR(ctx_id) << END_LOG;

                        //The word has to be known, otherwise it is an error situation
                        if (DO_SANITY_CHECKS && (*curr_wid_iter == WordIndexType::UNKNOWN_WORD_ID)) {
                            stringstream msg;
                            msg << "The " << SSTR(idx) << "'th word of the m-gram is unknown!";
                            throw Exception(msg.str());
                        }

                        if (get_ctx_id_func[idx](m_trie, *curr_wid_iter, ctx_id)) {
                            LOGGER(LOG_LEVEL) << "get_context_id(" << SSTR(*curr_wid_iter)
                                    << ", prevCtxId) = " << SSTR(ctx_id) << END_LOG;
                            curr_wid_iter++;
                            idx++;
                        } else {
                            //The next context id could not be retrieved
                            return false;
                        }
                    }

                    LOGGER(LOG_LEVEL) << "Resulting m-gram context id: " << SSTR(ctx_id) << END_LOG;

                    return true;
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
                inline void get_context_id(const T_Model_M_Gram<WordIndexType> &gram, TLongId &ctx_id) {
                    //Perform sanity check for the level values they should be the same!
                    if (DO_SANITY_CHECKS && (CURR_LEVEL != gram.get_m_gram_level())) {
                        stringstream msg;
                        msg << "The improper level values! Template level parameter = " << SSTR(CURR_LEVEL)
                                << " but the m-gram level value is: " << SSTR(gram.get_m_gram_level());
                        throw Exception(msg.str());
                    }

                    //Try to retrieve the context from the cache, if not present then compute it
                    if (get_cached_context_id(gram, ctx_id)) {
                        //Compute the context id
                        if (!get_m_gram_ctx_id<LOG_LEVEL>(gram.first(), gram.last(), ctx_id)) {
                            //The next context id could not be computed
                            stringstream msg;
                            msg << "The m-gram:" << (string) gram << " context could not be computed!";
                            throw Exception(msg.str());
                        }

                        //Cache the newly computed context id for the given n-gram context
                        set_cache_context_id(gram, ctx_id);

                        //The context Id was found in the Trie
                        LOGGER(LOG_LEVEL) << "The ctxId could be computed, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    } else {
                        //The context Id was found in the cache
                        LOGGER(LOG_LEVEL) << "The ctxId was found in cache, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
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
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
                        return false;
                    } else {
                        LOG_DEBUG2 << "Cache MISS! [" << m_chached_ctx << "] != [" << gram.m_context
                                << "], for m-gram: " << (string) gram
                                << ", cached ctxId: " << SSTR(m_chached_ctx_id) << END_LOG;
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

                //Stores the pointers to instances of th get_ctx_id function templates
                static const TGetCtxIdFunct get_ctx_id_func[];
            };

            template<typename TrieType>
            constexpr TModelLevel LayeredTrieDriver<TrieType>::MAX_LEVEL;

            template<typename TrieType>
            const typename LayeredTrieDriver<TrieType>::TGetCtxIdFunct LayeredTrieDriver<TrieType>::get_ctx_id_func[] = {
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

