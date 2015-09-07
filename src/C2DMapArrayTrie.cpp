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

using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            C2DMapArrayTrie<N>::C2DMapArrayTrie(AWordIndex * const _pWordIndex,
                    const float _mGramMemFactor,
                    const float _nGramMemFactor)
            : ALayeredTrie<N>(_pWordIndex,
            [&] (const TShortId wordId, TLongId & ctxId, const TModelLevel level) -> bool {

                return C2DMapArrayTrie<N>::getContextId(wordId, ctxId, level); }),
            mGramMemFactor(_mGramMemFactor),
            nGramMemFactor(_nGramMemFactor),
            m_1_gram_data(NULL) {

                //Perform an error check! This container has a lower bound on the N level.
                if (N < M_GRAM_LEVEL_2) {

                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << M_GRAM_LEVEL_2 << "!";
                    throw Exception(msg.str());
                }

                //Memset the M grams reference and data arrays
                memset(pMGramAlloc, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TMGramAllocator *));
                memset(pMGramMap, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TMGramsMap *));
                memset(m_M_gram_data, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TProbBackOffEntry *));

                //Initialize the array of counters
                memset(m_M_gram_num_ctx_ids, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TShortId));
                memset(m_M_gram_next_ctx_id, 0, ALayeredTrie<N>::NUM_M_GRAM_LEVELS * sizeof (TShortId));
                
                //Initialize the N-gram level data
                pNGramAlloc = NULL;
                pNGramMap = NULL;
                
                LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
            }

            template<TModelLevel N>
            void C2DMapArrayTrie<N>::preAllocateOGrams(const size_t counts[N]) {
                //Compute the number of words to be stored
                const size_t num_word_ids = ATrie<N>::get_word_index()->get_words_count(counts[0]);

                //Pre-allocate the 1-Gram data
                m_1_gram_data = new TProbBackOffEntry[num_word_ids];
                memset(m_1_gram_data, 0, num_word_ids * sizeof (TProbBackOffEntry));


                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntry & pbData = m_1_gram_data[AWordIndex::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;
            }

            template<TModelLevel N>
            void C2DMapArrayTrie<N>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 0; idx < ALayeredTrie<N>::NUM_M_GRAM_LEVELS; idx++) {
                    //Get the number of elements to pre-allocate

                    //Get the number of the M-grams on this level
                    const uint num_grams = counts[idx+1];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx], num_grams, "M-Grams", mGramMemFactor);

                    //Get the number of M-gram indexes on this level
                    const uint num_ngram_idx = m_M_gram_num_ctx_ids[idx];
                    
                    m_M_gram_data[idx] = new TProbBackOffEntry[num_ngram_idx];
                    memset(m_M_gram_data[idx], 0, num_ngram_idx * sizeof (TProbBackOffEntry));
                }
            }

            template<TModelLevel N>
            void C2DMapArrayTrie<N>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", nGramMemFactor);
            }

            template<TModelLevel N>
            void C2DMapArrayTrie<N>::pre_allocate(const size_t counts[N]) {
                //Call the super class pre-allocator!
                ALayeredTrie<N>::pre_allocate(counts);
                
                //Compute and store the M-gram level sizes in terms of the number of M-gram indexes per level
                //Also initialize the M-gram index counters, for issuing context indexes
                for (TModelLevel i = 0; i < ALayeredTrie<N>::NUM_M_GRAM_LEVELS; i++) {
                    //The index counts must start with one as zero is reserved for the UNDEFINED_ARR_IDX
                    m_M_gram_next_ctx_id[i] = ALayeredTrie<N>::FIRST_VALID_CTX_ID;
                    //Due to the reserved first index, make the array sizes one element larger, to avoid extra computations
                    m_M_gram_num_ctx_ids[i] = counts[i + 1] + ALayeredTrie<N>::FIRST_VALID_CTX_ID;
                }

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N>
            C2DMapArrayTrie<N>::~C2DMapArrayTrie() {
                //Deallocate One-Grams
                if (m_1_gram_data != NULL) {
                    delete[] m_1_gram_data;
                }

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < ALayeredTrie<N>::NUM_M_GRAM_LEVELS; idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx]);
                    delete[] m_M_gram_data[idx];
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class C2DMapArrayTrie<M_GRAM_LEVEL_MAX>;
        }
    }
}