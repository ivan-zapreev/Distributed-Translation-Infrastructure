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

using namespace uva::smt::utils::text;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            C2DHashMapTrie<N>::C2DHashMapTrie(AWordIndex * const _pWordIndex,
                    const float _mGramMemFactor,
                    const float _nGramMemFactor)
            : ALayeredTrie<N>(_pWordIndex,
            [] (const TShortId wordId, TLongId & ctxId, const TModelLevel level) -> bool {

                return C2DHashMapTrie<N>::getContextId(wordId, ctxId, level); }),
            mGramMemFactor(_mGramMemFactor),
            nGramMemFactor(_nGramMemFactor),
                        m_1_gram_data(NULL) {
                if (DO_SANITY_CHECKS) {
                    //Initialize the hash statistics map
                    for (int i = 0; i < N; i++) {
                        hashSizes[i].first = UINT64_MAX;
                        hashSizes[i].second = 0;
                    }
                }

                //Perform an error check! This container has a lower bound on the N level.
                if (N < TWO_GRAM_LEVEL) {

                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << TWO_GRAM_LEVEL << "!";
                    throw Exception(msg.str());
                }

                LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
            }

            template<TModelLevel N>
            void C2DHashMapTrie<N>::preAllocateOGrams(const size_t counts[N]) {
                //Compute the number of words to be stored

                const size_t num_word_ids = counts[0] + ALayeredTrie<N>::EXTRA_NUMBER_OF_WORD_IDs; //Add an extra element(3) for the <unknown/> word

                //Pre-allocate the 1-Gram data
                m_1_gram_data = new TProbBackOffEntry[num_word_ids];
                memset(m_1_gram_data, 0, num_word_ids * sizeof (TProbBackOffEntry));
                

                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntry & pbData = m_1_gram_data[UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back_off = ZERO_BACK_OFF_WEIGHT;
            }

            template<TModelLevel N>
            void C2DHashMapTrie<N>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 1; idx < (N - 1); idx++) {
                    //Get the number of elements to pre-allocate

                    const uint numEntries = counts[idx];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx - 1], &pMGramAlloc[idx - 1], numEntries, "M-Grams", mGramMemFactor);
                }
            }

            template<TModelLevel N>
            void C2DHashMapTrie<N>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", nGramMemFactor);
            }

            template<TModelLevel N>
            void C2DHashMapTrie<N>::preAllocate(const size_t counts[N]) {
                //Call the super class pre-allocator!

                ALayeredTrie<N>::preAllocate(counts);

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N>
            C2DHashMapTrie<N>::~C2DHashMapTrie() {
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
            template class C2DHashMapTrie<MAX_NGRAM_LEVEL>;
        }
    }
}