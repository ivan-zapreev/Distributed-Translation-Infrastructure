/* 
 * File:   HybridMemoryTrie.hpp
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
 * Created on August 21, 2015, 4:18 PM
 */

#ifndef HYBRIDMEMORYTRIE_HPP
#define	HYBRIDMEMORYTRIE_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "TextPieceReader.hpp"
#include "AWordIndex.hpp"
#include "HybridMemoryTrieStorage.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the hybrid memory trie implementation class. It has two template parameters.
             * @param N the maximum number of levelns in the trie.
             * @param C the container class to store context-to-prob_back_off_index pairs, must derive from ACtxToPBStorare
             */
            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            class HybridMemoryTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit HybridMemoryTrie(AWordIndex * const p_word_index);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * This method will get the N-gram in a form of a vector, e.g.:
                 *      [word1 word2 word3 word4 word5]
                 * and will compute and return the Language Model Probability for it
                 * For more details @see ATrie
                 */
                virtual void queryNGram(const vector<string> & ngram, SProbResult & result);

                /**
                 * The basic destructor
                 */
                virtual ~HybridMemoryTrie();

            protected:

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                TProbBackOffEntryPair & make_1_GramDataRef(const TWordId wordId) {
                    //Get the word probability and back-off data reference
                    return m_mgram_data[0][wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TWordId wordId, const TContextId ctxId) {
                    StorageContainer*& ctx_mapping = m_mgram_mapping[level - MGRAM_MAPPING_IDX_OFFSET][wordId];
                    if (ctx_mapping == NULL) {
                        ctx_mapping = m_storage_factory->create(level);
                        LOG_DEBUG3 << "A new ACtxToPBStorage container is allocated for level " << level << END_LOG;
                    }
                    TIdIndex & nextCtxId = (*ctx_mapping)[ctxId];
                    nextCtxId = next_ctx_id[level - MGRAM_MAPPING_IDX_OFFSET]++;
                    return m_mgram_data[level - 1][nextCtxId];
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                TLogProbBackOff& make_N_GramDataRef(const TWordId wordId, const TContextId ctxId) {
                    StorageContainer*& ctx_mapping = m_mgram_mapping[N - MGRAM_MAPPING_IDX_OFFSET][wordId];
                    if (ctx_mapping == NULL) {
                        ctx_mapping = m_storage_factory->create(N);
                        LOG_DEBUG3 << "A new ACtxToPBStorage container is allocated for level " << N << END_LOG;
                    }
                    return (TLogProbBackOff &) (*ctx_mapping)[ctxId];
                };

            private:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_MAPPING_IDX_OFFSET = 2;

                //Stores the number of words
                size_t m_word_arr_size;

                //The factory to produce the storage containers
                StorageFactory<N> * m_storage_factory;

                //M-Gram data for 1 <= M < N. This is a 2D array storing
                //For each M-Gram level M an array of prob-back_off values
                // m_mgram_data[M][0] - probability/back-off pair for the given M-gram
                // m_mgram_data[M][1] --//--
                // ...
                // m_mgram_data[M][#M-Grams - 1] --//--
                // m_mgram_data[M][#M-Grams] --//--
                TProbBackOffEntryPair * m_mgram_data[N - 1];

                //M-Gram data for 1 < M <= N. This is a 2D array storing
                //For each M-Gram level M an array of #words elements of
                //pointers to C template parameter type:
                // m_mgram_mapping[M][0] -> NULL (if there is no M-gram ending with word 0 in the level)
                // m_mgram_mappin[M]g[1] -> C instance
                // ...
                // m_mgram_mapping[M][#words - 1] -> NULL
                // m_mgram_mapping[M][#words] -> C instance
                //
                //For all 1 < M < N instances of C contain mappings from the
                //context index to the index in the m_mgram_data[M] array -
                //the array storing probability/back-off values.
                //For M = N the pair value stored in the instance of C is the
                //probability itself! Note that that the probabilities are
                //stored as floats - 4 bytes and m_mgram_data[M] array is also a
                //4 byte integer, so we minimize memory usage by storing float
                //probability in place of the index.
                StorageContainer** m_mgram_mapping[N - 1];

                //Will store the next context index counters per M-gram level
                //for 1 < M < N.
                const static TModelLevel NUM_IDX_COUNTERS = N - 2;
                TIdIndex next_ctx_id[NUM_IDX_COUNTERS];

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level M > 1!
                 * 
                 * @param wordId the current word id
                 * @param ctxId the previous context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 */
                inline TContextId getContextId(TWordId wordId, TContextId ctxId, const TModelLevel level) {
                    return m_mgram_mapping[level - 1][wordId]->at(ctxId);
                }

            };

            typedef HybridMemoryTrie<MAX_NGRAM_LEVEL, CtxToPBUnorderedMapStorageFactory, CtxToPBUnorderedMapStorage> TFiveMapHybridMemoryTrie;
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

