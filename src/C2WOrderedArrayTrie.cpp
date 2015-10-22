/* 
 * File:   C2WOrderedArrayTrie.cpp
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
 * Created on August 25, 2015, 11:27 PM
 */
#include "C2WOrderedArrayTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, typename WordIndexType>
            C2WArrayTrie<N, WordIndexType>::C2WArrayTrie(WordIndexType & word_index)
            : LayeredTrieBase<N, WordIndexType>(word_index),
            m_1_gram_data(NULL), m_N_gram_data(NULL), m_one_gram_arr_size(0) {

                //Memset the M grams reference and data arrays
                memset(m_M_gram_ctx_2_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TSubArrReference *));
                memset(m_M_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TWordIdPBEntry *));

                //Initialize the array of counters
                memset(m_M_N_gram_num_ctx_ids, 0, BASE::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_N_gram_next_ctx_id, 0, BASE::NUM_M_N_GRAM_LEVELS * sizeof (TShortId));
            }

            template<TModelLevel N, typename WordIndexType>
            void C2WArrayTrie<N, WordIndexType>::pre_allocate(const size_t counts[N]) {
                //01) Pre-allocate the word index super class call
                BASE::pre_allocate(counts);

                //02) Compute and store the M-gram level sizes in terms of the number of M-grams per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < BASE::NUM_M_N_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_N_gram_next_ctx_id[i] = BASE::FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_N_gram_num_ctx_ids[i] = counts[i + 1] + BASE::FIRST_VALID_CTX_ID;
                }

                //03) Pre-allocate the 1-Gram data
                //The size of this array is made two elements larger than the number
                //of 1-Grams is since we want to account for the word indexes that start
                //from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                m_one_gram_arr_size = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new TMGramPayload[m_one_gram_arr_size];
                memset(m_1_gram_data, 0, m_one_gram_arr_size * sizeof (TMGramPayload));

                //04) Insert the unknown word data into the allocated array
                TMGramPayload & pbData = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                //05) Allocate data for the M-grams

                //First allocate the contexts to data mappings for the 2-grams (stored under index 0)
                //The number of contexts is the number of words in previous level 1 i.e. counts[0]
                //Yet we know that the word index begins with 2, due to UNDEFINED and UNKNOWN word ids
                //Therefore for the 2-gram level contexts array we add two more elements, just to simplify computations
                m_M_gram_ctx_2_data[0] = new TSubArrReference[m_one_gram_arr_size];
                memset(m_M_gram_ctx_2_data[0], 0, m_one_gram_arr_size * sizeof (TSubArrReference));

                //Now also allocate the data for the 2-Grams, the number of 2-grams is m_MN_gram_size[0] 
                m_M_gram_data[0] = new TWordIdPBEntry[m_M_N_gram_num_ctx_ids[0]];
                memset(m_M_gram_data[0], 0, m_M_N_gram_num_ctx_ids[0] * sizeof (TWordIdPBEntry));

                //Now the remaining elements can be added in a loop
                for (TModelLevel i = 1; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                    //Here i is the index of the array, the corresponding M-gram
                    //level M = i + 2. The m_MN_gram_size[i-1] stores the number of elements
                    //on the previous level - the maximum number of possible contexts.
                    m_M_gram_ctx_2_data[i] = new TSubArrReference[m_M_N_gram_num_ctx_ids[i - 1]];
                    memset(m_M_gram_ctx_2_data[i], 0, m_M_N_gram_num_ctx_ids[i - 1] * sizeof (TSubArrReference));
                    //The m_MN_gram_size[i] stores the number of elements
                    //on the current level - the number of M-Grams.
                    m_M_gram_data[i] = new TWordIdPBEntry[m_M_N_gram_num_ctx_ids[i]];
                    memset(m_M_gram_data[i], 0, m_M_N_gram_num_ctx_ids[i] * sizeof (TWordIdPBEntry));
                }

                //06) Allocate the data for the N-Grams.
                m_N_gram_data = new TCtxIdProbEntry[m_M_N_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR]];
                memset(m_N_gram_data, 0, m_M_N_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR] * sizeof (TCtxIdProbEntry));
            }

            template<TModelLevel N, typename WordIndexType>
                template<TModelLevel level>
            bool C2WArrayTrie<N, WordIndexType>::get_ctx_id(const TShortId wordId, TLongId & ctxId) const {
                //Compute the m-gram index
                const TModelLevel mgram_idx = level - BASE::MGRAM_IDX_OFFSET;

                if (DO_SANITY_CHECKS && ((level == N) || (mgram_idx < 0))) {
                    stringstream msg;
                    msg << "Unsupported level id: " << level;
                    throw Exception(msg.str());
                }

                LOG_DEBUG2 << "Searching for the next ctxId of " << SSTR(level)
                        << "-gram with wordId: " << SSTR(wordId) << ", ctxId: "
                        << SSTR(ctxId) << END_LOG;

                //First get the sub-array reference. Note that, even if it is the 2-Gram
                //case and the previous word is unknown (ctxId == 0) we still can use
                //the ctxId to get the data entry. The reason is that we allocated memory
                //for it but being for an unknown word context it should have no data!
                TSubArrReference & ref = m_M_gram_ctx_2_data[mgram_idx][ctxId];

                LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                        << ", with beginIdx: " << SSTR(ref.beginIdx) << ", endIdx: "
                        << SSTR(ref.endIdx) << END_LOG;

                //Check that there is data for the given context available
                if (ref.beginIdx != BASE::UNDEFINED_ARR_IDX) {
                    TShortId nextCtxId = BASE::UNDEFINED_ARR_IDX;
                    //The data is available search for the word index in the array
                    //WARNING: The linear search here is much slower!!!
                    if (my_bsearch_id<TWordIdPBEntry>(m_M_gram_data[mgram_idx], ref.beginIdx, ref.endIdx, wordId, nextCtxId)) {
                        LOG_DEBUG2 << "The next ctxId for wordId: " << SSTR(wordId) << ", ctxId: "
                                << SSTR(ctxId) << " is nextCtxId: " << SSTR(nextCtxId) << END_LOG;
                        ctxId = nextCtxId;
                        return true;
                    } else {
                        LOG_DEBUG2 << "Unable to find M-gram ctxId for level: "
                                << SSTR(level) << ", prev ctxId: " << SSTR(ctxId)
                                << ", wordId: " << SSTR(wordId) << ", is not in the available range: ["
                                << SSTR(m_M_gram_data[mgram_idx][ref.beginIdx].id) << " ... "
                                << SSTR(m_M_gram_data[mgram_idx][ref.endIdx].id) << "]" << END_LOG;
                        return false;
                    }
                } else {
                    LOG_DEBUG2 << "Unable to find M-gram context id for level: "
                            << SSTR(level) << ", prev ctxId: " << SSTR(ctxId)
                            << ", nothing present in that context!" << END_LOG;
                    return false;
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            C2WArrayTrie<MAX_LEVEL, WordIndexType>::~C2WArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_M_gram_ctx_2_data[i];
                        delete[] m_M_gram_data[i];
                    }
                    delete[] m_N_gram_data;
                }
            }

            template<TModelLevel N, typename WordIndexType>
            TMGramPayload & C2WArrayTrie<N, WordIndexType>::make_1_gram_data_ref(const TShortId wordId) {
                LOG_DEBUG2 << "Adding 1-gram with wordId: " << SSTR(wordId) << END_LOG;
                return m_1_gram_data[wordId];
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2WArrayTrie<N, WordIndexType>::get_1_gram_data_ref(const TShortId wordId, const TMGramPayload ** ppData) const {
                LOG_DEBUG2 << "Getting 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                *ppData = &m_1_gram_data[wordId];

                //The data should always be present, unless of course this is a bad index!
                return true;
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            TMGramPayload& C2WArrayTrie<N, WordIndexType>::make_m_gram_data_ref(const TShortId wordId, const TLongId ctxId) {
                //Compute the m-gram index
                const TModelLevel mgram_idx = level - BASE::MGRAM_IDX_OFFSET;

                LOG_DEBUG2 << "Adding " << SSTR(level) << "-gram with ctxId: "
                        << SSTR(ctxId) << ", wordId: " << SSTR(wordId) << END_LOG;

                //First get the sub-array reference. 
                TSubArrReference & ref = m_M_gram_ctx_2_data[mgram_idx][ctxId];

                //Check that the array is continuous in indexes, so that we add
                //context after context and not switching between different contexts!
                if (DO_SANITY_CHECKS && (ref.endIdx != BASE::UNDEFINED_ARR_IDX)
                        && (ref.endIdx + 1 != m_M_N_gram_next_ctx_id[mgram_idx])) {
                    stringstream msg;
                    msg << "The " << SSTR(level) << " -gram ctxId: " << SSTR(ctxId)
                            << " array is not ordered ref.endIdx = " << SSTR(ref.endIdx)
                            << ", next ref.endIdx = " << SSTR(m_M_N_gram_next_ctx_id[mgram_idx] + 1) << "!";
                    throw Exception(msg.str());
                }

                //Get the new index and increment - this will be the new end index
                ref.endIdx = m_M_N_gram_next_ctx_id[mgram_idx]++;

                //Check if we exceeded the maximum allowed number of M-grams
                if (DO_SANITY_CHECKS && (ref.endIdx >= m_M_N_gram_num_ctx_ids[mgram_idx])) {
                    stringstream msg;
                    msg << "The maximum allowed number of " << SSTR(level) << "-grams: "
                            << SSTR(m_M_N_gram_num_ctx_ids[mgram_idx]) << " is exceeded )!";
                    throw Exception(msg.str());
                }

                //Check if there are yet no elements for this context
                if (ref.beginIdx == BASE::UNDEFINED_ARR_IDX) {
                    //There was no elements put into this context, the begin index is then equal to the end index
                    ref.beginIdx = ref.endIdx;
                }

                //Store the word id
                m_M_gram_data[mgram_idx][ref.endIdx].id = wordId;

                //Return the reference to the newly allocated element
                return m_M_gram_data[mgram_idx][ref.endIdx].data;
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            bool C2WArrayTrie<N, WordIndexType>::get_m_gram_data_ref(const TShortId wordId,
                    TLongId ctxId, const TMGramPayload **ppData) const {
                //Compute the m-gram index
                const TModelLevel mgram_idx = level - BASE::MGRAM_IDX_OFFSET;

                LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                        << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                //Get the context id, note we use short ids here!
                if (get_ctx_id<level>(wordId, ctxId)) {
                    //Return the data by the context
                    *ppData = &m_M_gram_data[mgram_idx][ctxId].data;
                    return true;
                } else {
                    //The data could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            TLogProbBackOff& C2WArrayTrie<N, WordIndexType>::make_n_gram_data_ref(const TShortId wordId, const TLongId ctxId) {
                //Get the new n-gram index
                const TShortId n_gram_idx = m_M_N_gram_next_ctx_id[BASE::N_GRAM_IDX_IN_M_N_ARR]++;

                LOG_DEBUG2 << "Adding " << SSTR(N) << "-gram with ctxId: " << SSTR(ctxId)
                        << ", wordId: " << SSTR(wordId) << " @ index: " << SSTR(n_gram_idx) << END_LOG;

                //Check if we exceeded the maximum allowed number of M-grams
                if (DO_SANITY_CHECKS && (n_gram_idx >= m_M_N_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR])) {
                    stringstream msg;
                    const TShortId max_num = m_M_N_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR];
                    msg << "The maximum allowed number of " << SSTR(N) << "-grams: "
                            << SSTR(max_num) << " is exceeded )!";
                    throw Exception(msg.str());
                }

                //Store the context and word ids
                m_N_gram_data[n_gram_idx].ctxId = ctxId;
                m_N_gram_data[n_gram_idx].wordId = wordId;

                //Create the search key by combining ctx and word ids, see TCtxIdProbEntryPair
                const TLongId key = TShortId_TShortId_2_TLongId(wordId, ctxId);
                LOG_DEBUG4 << "Storing N-Gram: TShortId_TShortId_2_TLongId(wordId = " << SSTR(wordId)
                        << ", ctxId = " << SSTR(ctxId) << ") = " << SSTR(key) << END_LOG;

                //return the reference to the probability
                return m_N_gram_data[n_gram_idx].prob;
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2WArrayTrie<N, WordIndexType>::get_n_gram_data_ref(const TShortId wordId, const TLongId ctxId,
                    TLogProbBackOff & prob) const {
                LOG_DEBUG2 << "Getting " << SSTR(N) << "-gram with wordId: "
                        << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                //Create the search key by combining ctx and word ids, see TCtxIdProbEntryPair
                const TLongId key = TShortId_TShortId_2_TLongId(wordId, ctxId);
                LOG_DEBUG4 << "Searching N-Gram: TShortId_TShortId_2_TLongId(wordId = " << SSTR(wordId)
                        << ", ctxId = " << SSTR(ctxId) << ") = " << SSTR(key) << END_LOG;

                //Search for the index using binary search
                TShortId idx = BASE::UNDEFINED_ARR_IDX;
                if (my_bsearch_wordId_ctxId<TCtxIdProbEntry>(m_N_gram_data, BASE::FIRST_VALID_CTX_ID,
                        m_M_N_gram_num_ctx_ids[BASE::N_GRAM_IDX_IN_M_N_ARR] - 1, wordId, ctxId, idx)) {
                    //return the reference to the probability
                    prob = m_N_gram_data[idx].prob;
                    return true;
                } else {
                    LOG_DEBUG1 << "Unable to find " << SSTR(N) << "-gram data for ctxId: "
                            << SSTR(ctxId) << ", wordId: " << SSTR(wordId)
                            << ", key " << SSTR(key) << END_LOG;
                    return false;
                }
            };

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2WArrayTrie, TOptCountWordIndex);
        }
    }
}

