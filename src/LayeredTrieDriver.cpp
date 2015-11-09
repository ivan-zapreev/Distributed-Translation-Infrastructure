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
            void LayeredTrieDriver<TrieType>::add_1_gram(const T_Model_M_Gram<WordIndexType> &gram) {
                //Get the word id of this unigram, so there is just one word in it and its the end one
                const TShortId word_id = gram.get_end_word_id();

                //Get the word probability and back-off data reference
                T_M_Gram_Payload & pbData = m_trie.make_1_gram_data_ref(word_id);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (pbData.prob != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!
                    REPORT_COLLISION_WARNING(gram,
                            word_id, WordIndexType::UNDEFINED_WORD_ID,
                            pbData.prob, pbData.back,
                            gram.m_prob, gram.m_back_off);
                }

                //Set/Update the probability and back-off values for the word
                pbData.prob = gram.m_prob;
                pbData.back = gram.m_back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.prob << "," << pbData.back << ") for "
                        << (string) gram << " wordHash = " << word_id << END_LOG;
            };

            template<typename TrieType >
            template<TModelLevel CURR_LEVEL>
            void LayeredTrieDriver<TrieType>::add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                get_context_id<CURR_LEVEL, DebugLevelsEnum::DEBUG2>(gram, ctx_id);

                // 2. Insert the probability data into the trie
                TShortId end_word_id = gram.get_end_word_id();
                //The word has to be known, otherwise it is an error situation
                if (DO_SANITY_CHECKS && (end_word_id == WordIndexType::UNKNOWN_WORD_ID)) {
                    stringstream msg;
                    msg << "Could not get end wordId for " << (string) gram;
                    throw Exception(msg.str());
                }
                T_M_Gram_Payload& pbData = m_trie.template make_m_gram_data_ref<CURR_LEVEL>(end_word_id, ctx_id);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (pbData.prob != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!
                    REPORT_COLLISION_WARNING(gram, end_word_id, ctx_id,
                            pbData.prob, pbData.back,
                            gram.m_prob, gram.m_back_off);
                }

                //Set/Update the probability and back-off values for the word
                pbData.prob = gram.m_prob;
                pbData.back = gram.m_back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.prob << "," << pbData.back << ") for "
                        << (string) gram << " contextHash = "
                        << ctx_id << ", wordHash = " << end_word_id << END_LOG;
            };

            template<typename TrieType >
            void LayeredTrieDriver<TrieType>::add_n_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                get_context_id<MAX_LEVEL, DebugLevelsEnum::DEBUG2>(gram, ctx_id);

                // 2. Insert the probability data into the trie
                const TShortId end_word_id = gram.get_end_word_id();
                //The word has to be known, otherwise it is an error situation
                if (DO_SANITY_CHECKS && (end_word_id == WordIndexType::UNKNOWN_WORD_ID)) {
                    stringstream msg;
                    msg << "Could not get end wordId for " << (string) gram;
                    throw Exception(msg.str());
                }
                TLogProbBackOff& prob_ref = m_trie.make_n_gram_data_ref(end_word_id, ctx_id);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (prob_ref != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!

                    REPORT_COLLISION_WARNING(gram, end_word_id, ctx_id,
                            prob_ref, UNDEF_LOG_PROB_WEIGHT,
                            gram.m_prob, UNDEF_LOG_PROB_WEIGHT);
                }

                //Set/Update the probability
                prob_ref = gram.m_prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << prob_ref << ") for "
                        << (string) gram << " contextHash = "
                        << ctx_id << ", wordHash = " << end_word_id << END_LOG;
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
                if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram
                    TLongId ctx_id;

                    //Compute the context id based on what is stored in m_GramWordIds and context length
                    if (get_m_gram_ctx_id(gram.template get_word_id_ptr<BEGIN_WORD_IDX>(), gram.template get_word_id_ptr<END_WORD_IDX>(), ctx_id)) {
                        LOG_DEBUG << "Got query context id: " << ctx_id << END_LOG;
                        if (CURR_LEVEL == MAX_LEVEL) {
                            return m_trie.get_n_gram_payload(end_word_id, ctx_id, payload);
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N
                            //The context length plus one is M value of the M-Gram
                            return m_trie.template get_m_gram_payload<CURR_LEVEL>(end_word_id, ctx_id, payload);
                        }
                    } else {
                        //Could not compute the context id for the given level, so backing off (recursive)!
                        LOG_DEBUG << "Unable to find the " << SSTR(CURR_LEVEL) << "-Gram context id, need to back off!" << END_LOG;
                        return GPR_Enum::FAILED_GPR;
                    }
                } else {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    m_trie.get_1_gram_payload(end_word_id, payload);
                    return GPR_Enum::PAYLOAD_GPR;
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

