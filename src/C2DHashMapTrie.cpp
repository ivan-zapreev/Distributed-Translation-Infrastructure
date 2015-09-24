/* 
 * File:   ContextContextMultiHashMapTrie.cpp
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
 * Created on August 14, 2015, 1:53 PM
 */
#include "C2DHashMapTrie.hpp"

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
            C2DMapTrie<N, WordIndexType>::C2DMapTrie(
                    WordIndexType & word_index,
                    const float mgram_mem_factor,
                    const float ngram_mem_factor)
            : LayeredTrieBase<N, WordIndexType>(word_index),
            m_mgram_mem_factor(mgram_mem_factor),
            m_ngram_mem_factor(ngram_mem_factor),
            m_1_gram_data(NULL) {
                if (DO_SANITY_CHECKS) {
                    //Initialize the hash statistics map
                    for (int i = 0; i < N; i++) {
                        hashSizes[i].first = UINT64_MAX;
                        hashSizes[i].second = 0;
                    }
                }

                //Perform an error check! This container has a lower bound on the N level.
                if (N < M_GRAM_LEVEL_2) {

                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << M_GRAM_LEVEL_2 << "!";
                    throw Exception(msg.str());
                }
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DMapTrie<N, WordIndexType>::preAllocateOGrams(const size_t counts[N]) {
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
            void C2DMapTrie<N, WordIndexType>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 1; idx < (N - 1); idx++) {
                    //Get the number of elements to pre-allocate

                    const uint numEntries = counts[idx];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx - 1], &pMGramAlloc[idx - 1], numEntries, "M-Grams", m_mgram_mem_factor);
                }
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DMapTrie<N, WordIndexType>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", m_ngram_mem_factor);
            }

            template<TModelLevel N, typename WordIndexType>
            void C2DMapTrie<N, WordIndexType>::pre_allocate(const size_t counts[N]) {
                //Call the super class pre-allocator!

                BASE::pre_allocate(counts);

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N, typename WordIndexType>
            bool C2DMapTrie<N, WordIndexType>::get_ctx_id(const TShortId wordId, TLongId & ctxId, const TModelLevel level) const {
                //Use the Szudzik algorithm as it outperforms Cantor
                ctxId = szudzik(wordId, ctxId);
                //The context can always be computed
                return true;
            }

            template<TModelLevel N, typename WordIndexType>
            TProbBackOffEntry & C2DMapTrie<N, WordIndexType>::make_1_gram_data_ref(const TShortId wordId) {
                if (DO_SANITY_CHECKS) {
                    //Add hash key statistics
                    if (Logger::isRelevantLevel(DebugLevelsEnum::INFO3)) {
                        hashSizes[0].first = min<TLongId>(wordId, hashSizes[0].first);
                        hashSizes[0].second = max<TLongId>(wordId, hashSizes[0].second);
                    }
                }

                //Get the word probability and back-off data reference
                return m_1_gram_data[wordId];
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2DMapTrie<N, WordIndexType>::get_1_gram_data_ref(const TShortId wordId, const TProbBackOffEntry ** ppData) const {
                //The data is always present.
                *ppData = &m_1_gram_data[wordId];
                return true;
            };

            template<TModelLevel N, typename WordIndexType>
            TProbBackOffEntry & C2DMapTrie<N, WordIndexType>::make_m_gram_data_ref(const TModelLevel level, const TShortId wordId, TLongId ctxId) {
                //Store the N-tires from length 2 on and indexing starts
                //with 0, therefore "level-2". Get/Create the mapping for this
                //word in the Trie level of the N-gram

                //Note: there is no need to check on the result of the function
                //as in this Trie the context can always be computed!
                (void) get_ctx_id(wordId, ctxId);

                //Add hash key statistics
                if (DO_SANITY_CHECKS && Logger::isRelevantLevel(DebugLevelsEnum::INFO3)) {
                    hashSizes[level - 1].first = min<TLongId>(ctxId, hashSizes[level - 1].first);
                    hashSizes[level - 1].second = max<TLongId>(ctxId, hashSizes[level - 1].second);
                }

                return pMGramMap[level - BASE::MGRAM_IDX_OFFSET]->operator[](ctxId);
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2DMapTrie<N, WordIndexType>::get_m_gram_data_ref(const TModelLevel level, const TShortId wordId,
                    TLongId ctxId, const TProbBackOffEntry **ppData) const {
                //Get the next context id
                if (get_ctx_id(wordId, ctxId)) {
                    //Search for the map for that context id
                    const TModelLevel idx = (level - BASE::MGRAM_IDX_OFFSET);
                    TMGramsMap::const_iterator result = pMGramMap[idx]->find(ctxId);
                    if (result == pMGramMap[idx]->end()) {
                        //There is no data found under this context
                        return false;
                    } else {
                        //There is data found under this context
                        *ppData = &result->second;
                        return true;
                    }
                } else {
                    //The context id could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            TLogProbBackOff & C2DMapTrie<N, WordIndexType>::make_n_gram_data_ref(const TShortId wordId, TLongId ctxId) {
                //Data stores the N-tires from length 2 on, therefore "idx-1"
                //Get/Create the mapping for this word in the Trie level of the N-gram

                //Note: there is no need to check on the result of the function
                //as in this Trie the context can always be computed!
                (void) get_ctx_id(wordId, ctxId);

                //Add hash key statistics
                if (DO_SANITY_CHECKS && Logger::isRelevantLevel(DebugLevelsEnum::INFO3)) {
                    hashSizes[N - 1].first = min<TLongId>(ctxId, hashSizes[N - 1].first);
                    hashSizes[N - 1].second = max<TLongId>(ctxId, hashSizes[N - 1].second);
                }

                return pNGramMap->operator[](ctxId);
            };

            template<TModelLevel N, typename WordIndexType>
            bool C2DMapTrie<N, WordIndexType>::get_n_gram_data_ref(const TShortId wordId, TLongId ctxId,
                    TLogProbBackOff & prob) const {
                //Get the next context id
                if (get_ctx_id(wordId, ctxId)) {
                    //Search for the map for that context id
                    TNGramsMap::const_iterator result = pNGramMap->find(ctxId);
                    if (result == pNGramMap->end()) {
                        //There is no data found under this context
                        return false;
                    } else {
                        //There is data found under this context
                        prob = result->second;
                        return true;
                    }
                } else {
                    //The context id could not be found
                    return false;
                }
            };

            template<TModelLevel N, typename WordIndexType>
            C2DMapTrie<N, WordIndexType>::~C2DMapTrie() {
                if (DO_SANITY_CHECKS) {
                    //Print the hash sizes statistics
                    for (int i = 0; i < N; i++) {
                        LOG_INFO3 << (i + 1) << "-Gram ctx hash [min,max]= [ " << hashSizes[i].first << ", " << hashSizes[i].second << " ]" << END_LOG;
                    }
                }

                //Deallocate One-Grams
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                }

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < (N - 2); idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx]);
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class C2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex >;
            template class C2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex>;
            template class C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<BasicWordIndex> >;
            template class C2DMapTrie<M_GRAM_LEVEL_MAX, OptimizingWordIndex<CountingWordIndex> >;
        }
    }
}