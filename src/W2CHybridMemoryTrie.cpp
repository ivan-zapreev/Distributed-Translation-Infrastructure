/* 
 * File:   HybridMemoryTrie.cpp
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
#include "W2CHybridMemoryTrie.hpp"

#include <inttypes.h>       // std::uint32_t

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            W2CHybridMemoryTrie<N, StorageFactory, StorageContainer>::W2CHybridMemoryTrie(AWordIndex * const p_word_index)
            : ATrie<N>(p_word_index,
            [&] (const TShortId wordId, const TLongId ctxId, const TModelLevel level) -> TLongId {

                return this->getContextId(wordId, ctxId, level); }),
            m_storage_factory(NULL) {
                //Check for the storage memory sized. This one is needed to be able to store
                //N-gram probabilities in the C type container as its value! See description
                //of the m_mgram_mapping data member.
                const size_t float_size = sizeof (TLogProbBackOff);
                const size_t idx_size = sizeof (TShortId);
                if (float_size != idx_size) {
                    stringstream msg;
                    msg << "Unable to use " << __FILE__ << " for a trie as it expects ( sizeof(TLogProbBackOff) = "
                            << float_size << " ) == ( sizeof(TIndexSize) = " << idx_size << ")!";
                    throw Exception(msg.str());
                }

                //Initialize the array of counters
                memset(next_ctx_id, 0, NUM_IDX_COUNTERS * sizeof (TShortId));
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void W2CHybridMemoryTrie<N, StorageFactory, StorageContainer>::preAllocate(const size_t counts[N]) {
                //Store the number of words plus 2 because a word with index 0 is
                //UNDEFINED and a word with index 1 is UNKNOWN (<unk>)
                m_word_arr_size = counts[0] + 2;

                //01) Pre-allocate the word index
                ATrie<N>::getWordIndex()->reserve(counts[0]);

                //02) Allocate the factory
                m_storage_factory = new StorageFactory<N>(counts);

                //03) Allocate the main arrays of pointers where probs/back-offs will be stored

                //First allocate the memory for the One-grams, add an extra
                //element for the unknown word and initialize it!
                m_mgram_data[0] = new TProbBackOffEntryPair[m_word_arr_size];
                memset(m_mgram_data[0], 0, m_word_arr_size * sizeof (TProbBackOffEntryPair));

                //Record the dummy probability and back-off values for the unknown word
                TProbBackOffEntryPair & pbData = m_mgram_data[0][UNKNOWN_WORD_ID];
                pbData.prob = MINIMAL_LOG_PROB_WEIGHT;
                pbData.back_off = UNDEFINED_LOG_PROB_WEIGHT;

                //Allocate more memory for probabilities and back off weight for
                //the remaining M-gram levels until M < N. For M==N there is no
                //back-off weights and thus we will store the probabilities just
                //Inside the C container class values.
                for (int idx = 1; idx < (N - 1); idx++) {
                    m_mgram_data[idx] = new TProbBackOffEntryPair[counts[idx]];
                    memset(m_mgram_data[idx], 0, counts[idx] * sizeof (TProbBackOffEntryPair));
                }

                //04) Allocate the word map arrays per level There is N-1 levels to have 
                //as the for M == 0 - the One Grams, we do not need this mappings
                for (int idx = 0; idx < (N - 1); idx++) {
                    m_mgram_mapping[idx] = new StorageContainer*[m_word_arr_size];
                    memset(m_mgram_mapping[idx], 0, m_word_arr_size * sizeof (StorageContainer*));
                }
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            void W2CHybridMemoryTrie<N, StorageFactory, StorageContainer>::queryNGram(const vector<string> & ngram, SProbResult & result) {
                //ToDo: Implement
                throw Exception("Not implemented: HybridMemoryTrie<N, StorageFactory, StorageContainer>::queryNGram(const vector<string> & ngram, SProbResult & result)");
            }

            template<TModelLevel N, template<TModelLevel > class StorageFactory, class StorageContainer>
            W2CHybridMemoryTrie<N, StorageFactory, StorageContainer>::~W2CHybridMemoryTrie() {
                //Delete the probability and back-off data
                for (TModelLevel idx = 0; idx < (N - 1); idx++) {
                    //Delete the prob/back-off arrays per level
                    if (m_mgram_data[idx] != NULL) {
                        delete[] m_mgram_data[idx];
                    }
                }
                //Delete the mapping data
                for (TModelLevel idx = 0; idx < (N - 1); idx++) {
                    //Delete the word arrays per level
                    if (m_mgram_mapping[idx] != NULL) {
                        for (TShortId widx = 0; widx < m_word_arr_size; widx++) {
                            //Delete the C containers per word index
                            if (m_mgram_mapping[idx][widx] != NULL) {
                                delete m_mgram_mapping[idx][widx];
                            }
                        }
                        delete[] m_mgram_mapping[idx];
                    }
                }
                if (m_storage_factory != NULL) {
                    delete m_storage_factory;
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class W2CHybridMemoryTrie<MAX_NGRAM_LEVEL, CtxToPBUnorderedMapStorageFactory, CtxToPBUnorderedMapStorage>;
        }
    }
}
