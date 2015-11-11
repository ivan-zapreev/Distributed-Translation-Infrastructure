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

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "GenericTrieBase.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
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
                        if (trie. template get_ctx_id<M_GRAM_LEVEL_2>(word_ids[1], ctx_id)) {
                            //If we are at least at a level 4 m-gram
                            if (CURR_LEVEL >= M_GRAM_LEVEL_4) {
                                if (GET_BACK_OFF_CTX_ID && (CURR_LEVEL == M_GRAM_LEVEL_4)) prev_ctx_id = ctx_id;
                                if (trie. template get_ctx_id<M_GRAM_LEVEL_3>(word_ids[2], ctx_id)) {
                                    //If we are at least at a level 5 m-gram
                                    if (CURR_LEVEL >= M_GRAM_LEVEL_5) {
                                        if (GET_BACK_OFF_CTX_ID && (CURR_LEVEL == M_GRAM_LEVEL_5)) prev_ctx_id = ctx_id;
                                        if (trie. template get_ctx_id<M_GRAM_LEVEL_4>(word_ids[3], ctx_id)) {
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
                 * @param gram the m-gram we need to compute the contex tfor. 
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
                    if ((CURR_LEVEL == TrieType::MAX_LEVEL) || trie.get_cached_context_id(gram, ctx_id)) {
                        //Compute the context id, check on the level
                        const TModelLevel ctx_level = search_m_gram_ctx_id<TrieType, CURR_LEVEL, false, LOG_LEVEL>(trie, gram.first_word_id(), ctx_id, ctx_id);

                        //Do sanity check if needed
                        ASSERT_SANITY_THROW((CURR_LEVEL - 1) != ctx_level, string("The m-gram:") +
                                ((string) gram) + string(" context could not be computed!"));

                        //Cache the newly computed context id for the given n-gram context
                        if (CURR_LEVEL != TrieType::MAX_LEVEL) trie.set_cache_context_id(gram, ctx_id);

                        //The context Id was found in the Trie
                        LOGGER(LOG_LEVEL) << "The ctx_id could be computed, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    } else {
                        //The context Id was found in the cache
                        LOGGER(LOG_LEVEL) << "The ctx_id was found in cache, " << "it's value is: " << SSTR(ctx_id) << END_LOG;
                    }
                }
            }

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class LayeredTrieBase : public GenericTrieBase<MAX_LEVEL, WordIndexType> {
            public:

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit LayeredTrieBase(WordIndexType & word_index)
                : GenericTrieBase<MAX_LEVEL, WordIndexType> (word_index),
                m_chached_ctx_id(WordIndexType::UNDEFINED_WORD_ID) {

                    //Clear the memory for the buffer and initialize it
                    memset(m_context_c_str, 0, MAX_N_GRAM_STRING_LENGTH * sizeof (char));
                    m_context_c_str[0] = '\0';

                    LOG_DEBUG3 << "Creating the TextPieceReader with a data ptr" << END_LOG;
                    m_chached_ctx.set(m_context_c_str, MAX_N_GRAM_STRING_LENGTH);
                }

                /**
                 * Allows to retrieve the payload for the One gram with the given Id.
                 * @param word_id the One-gram id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @throw nothing
                 */
                inline void get_1_gram_payload(const TShortId word_id,
                        T_M_Gram_Payload & payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * @param CURR_LEVEL the currently considered m-gram level
                 * @param word_id the id of the M-gram's last word
                 * @param ctx_id the M-gram context (the M-gram's prefix) id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @return true if the element was found, otherwise false
                 * @throw nothing
                 */
                template<TModelLevel CURR_LEVEL>
                inline GPR_Enum get_m_gram_payload(const TShortId word_id, TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to retrieve the payload for the N gram defined by the end word_id and ctx_id.
                 * @param word_id the id of the N-gram's last word
                 * @param ctx_id the N-gram context (the N-gram's prefix) id
                 * @param payload[out] the reference to the data that is to be set with the stored one.
                 * @return true if the probability was found, otherwise false
                 * @throw nothing
                 */
                inline GPR_Enum get_n_gram_payload(const TShortId word_id, const TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to get the the new context id for the word and previous context id given the level
                 * @param CURR_LEVEL the currently considered m-gram level
                 * @param word_id the word id on this level
                 * @param ctx_id the previous level context id
                 * @return true if computation of the next context is succeeded
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_ctx_id(const TShortId word_id, TLongId & ctx_id) const {
                    THROW_MUST_OVERRIDE();
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
                //The actual storage for the cached context c string
                char m_context_c_str[MAX_N_GRAM_STRING_LENGTH];
                //Stores the cached M-gram context (for 1 < M <= N )
                TextPieceReader m_chached_ctx;
                //Stores the cached M-gram context value (for 1 < M <= N )
                TLongId m_chached_ctx_id;
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, CountingWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptBasicWordIndex >;
            template class LayeredTrieBase<M_GRAM_LEVEL_MAX, TOptCountWordIndex >;

#define INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(CLASS_NAME, WORD_IDX_TYPE) \
            template class CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_1>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_2>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_3>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_4>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_5>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_6>(const TShortId word_id, TLongId & ctx_id) const; \
            template bool CLASS_NAME<M_GRAM_LEVEL_MAX, WORD_IDX_TYPE >::get_ctx_id<M_GRAM_LEVEL_7>(const TShortId word_id, TLongId & ctx_id) const;

        }
    }
}

#endif	/* LAYEREDTRIEBASE_HPP */

