/* 
 * File:   C2DMapArrayTrie.cpp
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
 * Created on September 1, 2015, 15:15 PM
 */
#include "C2DMapArrayTrie.hpp"

#include <stdexcept> //std::exception
#include <sstream>   //std::stringstream
#include <algorithm> //std::fill

#include "Logger.hpp"
#include "StringUtils.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, typename WordIndexType>
            C2DHybridTrie<N, WordIndexType>::C2DHybridTrie(WordIndexType & word_index,
                    const float mram_mem_factor,
                    const float ngram_mem_factor)
            : LayeredTrieBase<N, WordIndexType>(word_index),
            m_mgram_mem_factor(mram_mem_factor),
            m_ngram_mem_factor(ngram_mem_factor),
            m_1_gram_data(NULL) {

                //Perform an error check! This container has a lower bound on the N level.
                if (N < M_GRAM_LEVEL_2) {

                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << M_GRAM_LEVEL_2 << "!";
                    throw Exception(msg.str());
                }

                //Memset the M grams reference and data arrays
                memset(pMGramAlloc, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramAllocator *));
                memset(pMGramMap, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TMGramsMap *));
                memset(m_M_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffEntry *));

                //Initialize the array of counters
                memset(m_M_gram_num_ctx_ids, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_gram_next_ctx_id, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TShortId));

                //Initialize the N-gram level data
                pNGramAlloc = NULL;
                pNGramMap = NULL;
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DHybridTrie<N, WordIndexType>::preAllocateOGrams(const size_t counts[N]) {
                //Compute the number of words to be stored
                const size_t num_word_ids = BASE::get_word_index().get_number_of_words(counts[0]);

                //Pre-allocate the 1-Gram data
                m_1_gram_data = new TProbBackOffEntry[num_word_ids];
                memset(m_1_gram_data, 0, num_word_ids * sizeof (TProbBackOffEntry));


                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DHybridTrie<N, WordIndexType>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    //Get the number of elements to pre-allocate

                    //Get the number of the M-grams on this level
                    const uint num_grams = counts[idx + 1];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx], num_grams, "M-Grams", m_mgram_mem_factor);

                    //Get the number of M-gram indexes on this level
                    const uint num_ngram_idx = m_M_gram_num_ctx_ids[idx];

                    m_M_gram_data[idx] = new TProbBackOffEntry[num_ngram_idx];
                    memset(m_M_gram_data[idx], 0, num_ngram_idx * sizeof (TProbBackOffEntry));
                }
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DHybridTrie<N, WordIndexType>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", m_ngram_mem_factor);
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DHybridTrie<N, WordIndexType>::pre_allocate(const size_t counts[N]) {
                //Call the super class pre-allocator!
                BASE::pre_allocate(counts);

                //Compute and store the M-gram level sizes in terms of the number of M-gram indexes per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < BASE::NUM_M_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_gram_next_ctx_id[i] = BASE::FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_gram_num_ctx_ids[i] = counts[i + 1] + BASE::FIRST_VALID_CTX_ID;
                }

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N, typename WordIndexType>
            bool C2DHybridTrie<N, WordIndexType>::get_ctx_id(const TShortId wordId, TLongId & ctxId, const TModelLevel level) const {
                const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);

                //Search for the map for that context id
                const TModelLevel idx = level - BASE::MGRAM_IDX_OFFSET;
                TMGramsMap::const_iterator result = pMGramMap[idx]->find(key);
                if (result == pMGramMap[idx]->end()) {
                    //There is no data found under this context
                    return false;
                } else {
                    //Update the context with the found value uf the next context
                    ctxId = result->second;
                    //The context can always be computed
                    return true;
                }
            }

            template<TModelLevel N, typename WordIndexType>
            TProbBackOffEntry & C2DHybridTrie<N, WordIndexType>::make_1_gram_data_ref(const TShortId wordId) {
                //Get the word probability and back-off data reference
                return m_1_gram_data[wordId];
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2DHybridTrie<N, WordIndexType>::get_1_gram_data_ref(const TShortId wordId, const TProbBackOffEntry ** ppData) const {
                //The data is always present.
                *ppData = &m_1_gram_data[wordId];
                return true;
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            TProbBackOffEntry & C2DHybridTrie<N, WordIndexType>::make_m_gram_data_ref(const TShortId wordId, TLongId ctxId) {
                //Store the N-tires from length 2 on and indexing starts
                //with 0, therefore "level-2". Get/Create the mapping for this
                //word in the Trie level of the N-gram

                //Obtain the context key and then create a new mapping
                const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);

                //Get the next context id
                const TModelLevel idx = (level - BASE::MGRAM_IDX_OFFSET);
                TShortId nextCtxId = m_M_gram_next_ctx_id[idx]++;

                //Store the context mapping inside the map
                pMGramMap[idx]->operator[](key) = nextCtxId;

                //Return the reference to the piece of memory
                return m_M_gram_data[idx][nextCtxId];
            };

            template<TModelLevel N, typename WordIndexType>
            template<TModelLevel level>
            bool C2DHybridTrie<N, WordIndexType>::get_m_gram_data_ref(const TShortId wordId,
                    TLongId ctxId, const TProbBackOffEntry **ppData) const {
                //Get the next context id
                if (get_ctx_id(wordId, ctxId, level)) {
                    //There is data found under this context
                    *ppData = &m_M_gram_data[level - BASE::MGRAM_IDX_OFFSET][ctxId];
                    return true;
                } else {
                    //The context id could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            TLogProbBackOff & C2DHybridTrie<N, WordIndexType>::make_n_gram_data_ref(const TShortId wordId, TLongId ctxId) {
                const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);
                return pNGramMap->operator[](key);
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2DHybridTrie<N, WordIndexType>::get_n_gram_data_ref(const TShortId wordId, TLongId ctxId,
                    TLogProbBackOff & prob) const {
                const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);

                //Search for the map for that context id
                TNGramsMap::const_iterator result = pNGramMap->find(key);
                if (result == pNGramMap->end()) {
                    //There is no data found under this context
                    return false;
                } else {
                    //There is data found under this context
                    prob = result->second;
                    return true;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            C2DHybridTrie<N, WordIndexType>::~C2DHybridTrie() {
                //Deallocate One-Grams
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                }

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx]);
                    delete[] m_M_gram_data[idx];
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(C2DHybridTrie, TOptCountWordIndex);
        }
    }
}