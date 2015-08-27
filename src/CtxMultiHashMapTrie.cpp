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
#include "CtxMultiHashMapTrie.hpp"

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
            CtxMultiHashMapTrie<N>::CtxMultiHashMapTrie(AWordIndex * const _pWordIndex,
                    const float _oGramMemFactor,
                    const float _mGramMemFactor,
                    const float _nGramMemFactor)
            : ATrie<N>(_pWordIndex,
            [] (const TShortId wordId, const TLongId ctxId, const TModelLevel level) -> TLongId {

                return CtxMultiHashMapTrie<N>::getContextId(wordId, ctxId, level); }),
            oGramMemFactor(_oGramMemFactor),
            mGramMemFactor(_mGramMemFactor),
            nGramMemFactor(_nGramMemFactor) {
                //Initialize the hash statistics map
                for (int i = 0; i < N; i++) {
                    hashSizes[i].first = UINT64_MAX;
                    hashSizes[i].second = 0;
                }

                //Perform an error check! This container has a lower bound on the N level.
                if (N < BGRAM_LEVEL_VALUE) {

                    stringstream msg;
                    msg << "The requested N-gram level is '" << N
                            << "', but for '" << __FILE__ << "' it must be >= " << BGRAM_LEVEL_VALUE << "!";
                    throw Exception(msg.str());
                }

                LOG_INFO3 << "Using the <" << __FILE__ << "> model." << END_LOG;
            }

            template<TModelLevel N>
            void CtxMultiHashMapTrie<N>::preAllocateOGrams(const size_t counts[N]) {
                //Compute the number of words to be stored

                const size_t numEntries = counts[0] + 1; //Add an extra element for the <unknown/> word

                //Reserve the memory for the map
                reserve_mem_unordered_map<TOneGramsMap, TOneGramAllocator>(&pOneGramMap, &pOneGramAlloc, numEntries, "1-Grams", oGramMemFactor);

                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntry & pbData = pOneGramMap->operator[](UNKNOWN_WORD_ID);
                pbData.prob = MINIMAL_LOG_PROB_WEIGHT;
                pbData.back_off = UNDEFINED_LOG_PROB_WEIGHT;
            }

            template<TModelLevel N>
            void CtxMultiHashMapTrie<N>::preAllocateMGrams(const size_t counts[N]) {
                //Pre-allocate for the M-grams with 1 < M < N
                for (int idx = 1; idx < (N - 1); idx++) {
                    //Get the number of elements to pre-allocate

                    const uint numEntries = counts[idx];

                    //Reserve the memory for the map
                    reserve_mem_unordered_map<TMGramsMap, TMGramAllocator>(&pMGramMap[idx - 1], &pMGramAlloc[idx - 1], numEntries, "M-Grams", mGramMemFactor);
                }
            }

            template<TModelLevel N>
            void CtxMultiHashMapTrie<N>::preAllocateNGrams(const size_t counts[N]) {
                //Get the number of elements to pre-allocate

                const size_t numEntries = counts[N - 1];

                //Reserve the memory for the map
                reserve_mem_unordered_map<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc, numEntries, "N-Grams", nGramMemFactor);
            }

            template<TModelLevel N>
            void CtxMultiHashMapTrie<N>::preAllocate(const size_t counts[N]) {
                //Call the super class pre-allocator!

                ATrie<N>::preAllocate(counts);

                //Pre-allocate 0-Grams
                preAllocateOGrams(counts);

                //Pre-allocate M-Grams
                preAllocateMGrams(counts);

                //Pre-allocate N-Grams
                preAllocateNGrams(counts);
            }

            template<TModelLevel N>
            CtxMultiHashMapTrie<N>::~CtxMultiHashMapTrie() {
                //Print the hash sizes statistics
                for (int i = 0; i < N; i++) {
                    LOG_INFO3 << (i + 1) << "-Gram ctx hash [min,max]= [ " << hashSizes[i].first << ", " << hashSizes[i].second << " ]" << END_LOG;
                }

                //Deallocate One-Grams
                deallocate_container<TOneGramsMap, TOneGramAllocator>(&pOneGramMap, &pOneGramAlloc);

                //Deallocate M-Grams there are N-2 M-gram levels in the array
                for (int idx = 0; idx < (N - 2); idx++) {
                    deallocate_container<TMGramsMap, TMGramAllocator>(&pMGramMap[idx], &pMGramAlloc[idx]);
                }

                //Deallocate N-Grams
                deallocate_container<TNGramsMap, TNGramAllocator>(&pNGramMap, &pNGramAlloc);
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class CtxMultiHashMapTrie<MAX_NGRAM_LEVEL>;
        }
    }
}