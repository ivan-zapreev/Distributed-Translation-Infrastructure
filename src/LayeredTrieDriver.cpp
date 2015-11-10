/* 
 * File:   ATrie.cpp
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
 * Created on August 25, 2015, 29:27 PM
 */
#include "LayeredTrieDriver.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "C2DHashMapTrie.hpp"
#include "W2CHybridMemoryTrie.hpp"
#include "C2WOrderedArrayTrie.hpp"
#include "W2COrderedArrayTrie.hpp"
#include "C2DMapArrayTrie.hpp"

using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            template<typename TrieType >
            template<TModelLevel CURR_LEVEL>
            void LayeredTrieDriver<TrieType>::add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                    //Get the word id of this unigram, so there is just one word in it and its the end one
                    const TShortId word_id = gram.get_end_word_id();

                    //Add the m-gram payload
                    m_trie.template add_m_gram_payload<CURR_LEVEL>(word_id, WordIndexType::UNKNOWN_WORD_ID, gram.m_payload);
                } else {
                    // 1. Compute the context hash defined by w1 w2 w3
                    TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                    get_context_id<CURR_LEVEL, DebugLevelsEnum::DEBUG2>(gram, ctx_id);
                    // 2. Insert the probability data into the trie
                    TShortId end_word_id = gram.get_end_word_id();

                    //Add the m-gram payload
                    m_trie.template add_m_gram_payload<CURR_LEVEL>(end_word_id, ctx_id, gram.m_payload);
                }
            };

            template<typename TrieType >
            template<TModelLevel BEGIN_WORD_IDX, TModelLevel END_WORD_IDX, bool DO_BACK_OFF>
            GPR_Enum LayeredTrieDriver<TrieType>::get_payload(const T_Query_M_Gram<WordIndexType> & gram, T_M_Gram_Payload & payload, T_M_Gram_Payload & bo_payload) const {
                //Compute the current level from the begin and end word indexes
                constexpr TModelLevel CURR_LEVEL = (END_WORD_IDX - BEGIN_WORD_IDX) + 1;

                //Get the last word in the N-gram
                const TShortId & end_word_id = gram[END_WORD_IDX];

                LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(CURR_LEVEL) << "-gram ["
                        << SSTR(BEGIN_WORD_IDX) << ", " << SSTR(END_WORD_IDX) << "]" << END_LOG;

                //Consider different variants based no the length of the context
                if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    m_trie.get_1_gram_payload(end_word_id, payload);
                    return GPR_Enum::PAYLOAD_GPR;
                } else {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram
                    TLongId bo_ctx_id, ctx_id;

                    //Obtain the context id
                    if (search_m_gram_ctx_id<CURR_LEVEL, DO_BACK_OFF>(gram.template get_word_id_ptr<BEGIN_WORD_IDX>(), bo_ctx_id, ctx_id) == CURR_LEVEL) {
                        LOG_DEBUG << "Got query context id: " << ctx_id << ", back-off query context id: " << bo_ctx_id << END_LOG;
                        GPR_Enum result;
                        if (CURR_LEVEL == MAX_LEVEL) {
                            result = m_trie.get_n_gram_payload(end_word_id, ctx_id, payload);
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N
                            //The context length plus one is M value of the M-Gram
                            result = m_trie.template get_m_gram_payload<CURR_LEVEL>(end_word_id, ctx_id, payload);
                        }

                        //Check if we need to back-off and if the payload could
                        //not be obtained then try to get the back-off payload 
                        if (DO_BACK_OFF && (result == GPR_Enum::FAILED_GPR)) {
                            //Compute the back-off m-gram level
                            constexpr TModelLevel BO_CURR_LEVEL = CURR_LEVEL - 1;
                            LOG_DEBUG << "Asked to retrieve the back-off payload!" << END_LOG;
                            //Get the last word in the N-gram
                            const TShortId & bo_end_word_id = gram[END_WORD_IDX - 1];
                            if (BO_CURR_LEVEL == M_GRAM_LEVEL_1) {
                                //If we are looking for a 1-Gram probability, no need to compute the context
                                m_trie.get_1_gram_payload(bo_end_word_id, bo_payload);
                                return GPR_Enum::BACK_OFF_GPR;
                            } else {
                                if (m_trie.template get_m_gram_payload<BO_CURR_LEVEL>(bo_end_word_id, bo_ctx_id, bo_payload) == GPR_Enum::PAYLOAD_GPR) {
                                    return GPR_Enum::BACK_OFF_GPR;
                                }
                            }
                        }
                        return result;
                    } else {
                        //Could not compute the context id for the given level, this also
                        //means that there is no back-off payload present for this m-gram
                        LOG_DEBUG << "Unable to find the " << SSTR(CURR_LEVEL) << "-Gram context id, need to back off!" << END_LOG;
                        return GPR_Enum::FAILED_GPR;
                    }
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, TYPE) \
            INSTANTIATE_TRIE_TEMPLATE_TYPE(LayeredTrieDriver,T##TRIE_NAME##TYPE)

#define INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(TRIE_NAME) \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Basic); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, Count); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptBasic); \
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME_TYPE(TRIE_NAME, OptCount);

            /**************************************************************************/
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2DMapTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2WArrayTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(W2CArrayTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(W2CHybridTrie);
            INSTANTIATE_LAYERED_DRIVER_TEMPLATES_NAME(C2DHybridTrie);
            /**************************************************************************/

        }
    }
}

