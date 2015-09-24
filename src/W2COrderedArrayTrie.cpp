/* 
 * File:   W2COrderedArrayTrie.cpp
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
 * Created on August 27, 2015, 08:33 PM
 */
#include "W2COrderedArrayTrie.hpp"

#include <inttypes.h>   // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;

using namespace __W2CArrayTrie;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, typename WordIndexType>
            W2CArrayTrie<N, WordIndexType>::W2CArrayTrie(WordIndexType & word_index)
            : LayeredTrieBase<N, WordIndexType>(word_index),
            m_num_word_ids(0), m_1_gram_data(NULL), m_N_gram_word_2_data(NULL) {

                //Memset the M/N grams reference and data arrays
                memset(m_M_gram_word_2_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (T_M_GramWordEntry *));
            }

            template<TModelLevel N, typename WordIndexType>
            void W2CArrayTrie<N, WordIndexType>::pre_allocate(const size_t counts[N]) {
                //01) Pre-allocate the word index super class call
                BASE::pre_allocate(counts);

                //02) Pre-allocate the 1-Gram data
                m_num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);
                m_1_gram_data = new TProbBackOffEntry[m_num_word_ids];
                memset(m_1_gram_data, 0, m_num_word_ids * sizeof (TProbBackOffEntry));

                //03) Insert the unknown word data into the allocated array
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;

                //04) Allocate data for the M-grams

                for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                    preAllocateWordsData<T_M_GramWordEntry>(m_M_gram_word_2_data[i], counts[i + 1], counts[0]);
                }

                //05) Allocate the data for the N-Grams 
                preAllocateWordsData<T_N_GramWordEntry>(m_N_gram_word_2_data, counts[N - 1], counts[0]);
            }

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            bool W2CArrayTrie<N, WordIndexType>::get_ctx_id(const TShortId wordId, TLongId & ctxId) const {
                //Compute the m-gram index
                const TModelLevel mgram_idx = level - BASE::MGRAM_IDX_OFFSET;

                if (DO_SANITY_CHECKS && ((level == N) || (mgram_idx < 0))) {
                    stringstream msg;
                    msg << "Unsupported level id: " << level;
                    throw Exception(msg.str());
                }

                LOG_DEBUG2 << "Searching next ctxId for " << SSTR(level)
                        << "-gram with wordId: " << SSTR(wordId) << ", ctxId: "
                        << SSTR(ctxId) << END_LOG;

                //First get the sub-array reference. 
                const T_M_GramWordEntry & ref = m_M_gram_word_2_data[mgram_idx][wordId];

                if (DO_SANITY_CHECKS && ref.has_data()) {
                    LOG_DEBUG3 << "ref.size: " << SSTR(ref.size()) << ", ref.cio: "
                            << SSTR(ref.cio) << ", ctxId range: [" << SSTR(ref[0].id) << ", "
                            << SSTR(ref[ref.size() - 1].id) << "]" << END_LOG;
                }

                //Check that if this is the 2-Gram case and the previous context
                //id is 0 then it is the unknown word id, at least this is how it
                //is now in ATrie implementation, so we need to do a warning!
                if (DO_SANITY_CHECKS && (level == M_GRAM_LEVEL_2) && (ctxId < AWordIndex::MIN_KNOWN_WORD_ID)) {
                    LOG_WARNING << "Perhaps we are being paranoid but there "
                            << "seems to be a problem! The " << SSTR(level) << "-gram ctxId: "
                            << SSTR(ctxId) << " is equal to an undefined(" << SSTR(AWordIndex::UNDEFINED_WORD_ID)
                            << ") or unknown(" << SSTR(AWordIndex::UNKNOWN_WORD_ID) << ") word ids!" << END_LOG;
                }

                //Get the local entry index and then use it to compute the next context id
                typename T_M_GramWordEntry::TIndexType localIdx;
                //If the local entry index could be found then compute the next ctxId
                if (get_M_N_GramLocalEntryIdx(ref, ctxId, localIdx)) {
                    LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                            << ", size = " << SSTR(ref.size()) << ", localIdx = "
                            << SSTR(localIdx) << ", resulting ctxId = "
                            << SSTR(ref.cio + localIdx) << END_LOG;

                    //The next ctxId is the sum of the local index and the context index offset
                    ctxId = ref.cio + localIdx;
                    return true;
                } else {
                    //The local index could not be found
                    return false;
                }
            }

            template<TModelLevel N, typename WordIndexType>
            TProbBackOffEntry & W2CArrayTrie<N, WordIndexType>::make_1_gram_data_ref(const TShortId wordId) {
                LOG_DEBUG2 << "Adding 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                return m_1_gram_data[wordId];
            };

            template<TModelLevel N, typename WordIndexType>
            bool W2CArrayTrie<N, WordIndexType>::get_1_gram_data_ref(const TShortId wordId, const TProbBackOffEntry ** ppData) const {
                LOG_DEBUG2 << "Getting 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                *ppData = &m_1_gram_data[wordId];

                //The data should always be present, unless of course this is a bad index!

                return true;
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            TProbBackOffEntry& W2CArrayTrie<N, WordIndexType>::make_m_gram_data_ref(const TShortId wordId, const TLongId ctxId) {
                LOG_DEBUG2 << "Adding\t" << SSTR(level) << "-gram with ctxId:\t"
                        << SSTR(ctxId) << ", wordId:\t" << SSTR(wordId) << END_LOG;

                //Get the sub-array reference. 
                typename T_M_GramWordEntry::TElemType & ref = make_M_N_GramEntry<T_M_GramWordEntry>(m_M_gram_word_2_data[level - BASE::MGRAM_IDX_OFFSET], wordId);

                //Store the context and word ids
                ref.id = ctxId;

                //Return the reference to the newly allocated element

                return ref.payload;
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            bool W2CArrayTrie<N, WordIndexType>::get_m_gram_data_ref(const TShortId wordId,
                    const TLongId ctxId, const TProbBackOffEntry **ppData) const {
                LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                        << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                //Get the entry
                const typename T_M_GramWordEntry::TElemType * pEntry;
                if (get_M_N_GramEntry<T_M_GramWordEntry>(level, m_M_gram_word_2_data[level - BASE::MGRAM_IDX_OFFSET], wordId, ctxId, &pEntry)) {
                    //Return the pointer to the probability and back-off structure
                    *ppData = &pEntry->payload;
                    return true;
                } else {
                    //The data could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            TLogProbBackOff& W2CArrayTrie<N, WordIndexType>::make_n_gram_data_ref(const TShortId wordId, const TLongId ctxId) {
                LOG_DEBUG2 << "Adding " << SSTR(N) << "-gram with ctxId: "
                        << SSTR(ctxId) << ", wordId: " << SSTR(wordId) << END_LOG;

                //Get the sub-array reference. 
                typename T_N_GramWordEntry::TElemType & ref = make_M_N_GramEntry<T_N_GramWordEntry>(m_N_gram_word_2_data, wordId);

                //Store the context and word ids
                ref.id = ctxId;

                //Return the reference to the probability
                return ref.payload;
            };

            template<TModelLevel N, typename WordIndexType>
            bool W2CArrayTrie<N, WordIndexType>::get_n_gram_data_ref(const TShortId wordId, const TLongId ctxId,
                    TLogProbBackOff & prob) const {
                LOG_DEBUG2 << "Getting " << SSTR(N) << "-gram with wordId: "
                        << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                //Get the entry
                const typename T_N_GramWordEntry::TElemType * pEntry;
                if (get_M_N_GramEntry<T_N_GramWordEntry>(N, m_N_gram_word_2_data, wordId, ctxId, &pEntry)) {
                    //Return the reference to the probability
                    prob = pEntry->payload;
                    return true;
                } else {
                    //The data could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            W2CArrayTrie<N, WordIndexType>::~W2CArrayTrie() {
                //Check that the one grams were allocated, if yes then the rest must have been either
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                    for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                        delete[] m_M_gram_word_2_data[i];
                    }
                    delete[] m_N_gram_word_2_data;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CArrayTrie, TOptCountWordIndex);
        }
    }
}

