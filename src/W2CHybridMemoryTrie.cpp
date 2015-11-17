/* 
 * File:   W2CHybridMemoryTrie.cpp
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

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel MAX_LEVEL, typename WordIndexType, template<TModelLevel > class StorageFactory, class StorageContainer>
            W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>::W2CHybridTrie(WordIndexType & word_index)
            : LayeredTrieBase<W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>, MAX_LEVEL, WordIndexType, __W2CHybridTrie::DO_BITMAP_HASH_CACHE>(word_index),
            m_storage_factory(NULL) {
                //Perform an error check! This container has bounds on the supported trie level
                ASSERT_CONDITION_THROW((MAX_LEVEL < M_GRAM_LEVEL_2), string("The minimum supported trie level is") + std::to_string(M_GRAM_LEVEL_2));
                
                //Check for the storage memory sized. This one is needed to be able to store
                //N-gram probabilities in the C type container as its value! See description
                //of the m_mgram_mapping data member. We missuse the mapping container for
                //the last trie level by storing there probabilities instead of ids! 
                const size_t float_size = sizeof (TLogProbBackOff);
                const size_t idx_size = sizeof (TShortId);
                if (float_size > idx_size) {
                    stringstream msg;
                    msg << "Unable to use " << __FILE__ << " for a trie as it expects ( sizeof(TLogProbBackOff) = "
                            << float_size << " ) == ( sizeof(TIndexSize) = " << idx_size << ")!";
                    throw Exception(msg.str());
                }

                //Initialize the array of counters
                memset(next_ctx_id, 0, NUM_IDX_COUNTERS * sizeof (TShortId));
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType, template<TModelLevel > class StorageFactory, class StorageContainer>
            void W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>::pre_allocate(const size_t counts[MAX_LEVEL]) {
                //01) Pre-allocate the word index super class call
                BASE::pre_allocate(counts);

                //Compute the number of words to be stored
                m_word_arr_size = BASE::get_word_index().get_number_of_words(counts[0]);

                //02) Allocate the factory
                m_storage_factory = new StorageFactory<MAX_LEVEL>(counts);

                //03) Allocate the main arrays of pointers where probs/back-offs will be stored

                //First allocate the memory for the One-grams, add an extra
                //element for the unknown word and initialize it!
                m_mgram_data[0] = new T_M_Gram_Payload[m_word_arr_size];
                memset(m_mgram_data[0], 0, m_word_arr_size * sizeof (T_M_Gram_Payload));

                //Record the dummy probability and back-off values for the unknown word
                T_M_Gram_Payload & pbData = m_mgram_data[0][WordIndexType::UNKNOWN_WORD_ID];
                pbData.prob = UNK_WORD_LOG_PROB_WEIGHT;
                pbData.back = ZERO_BACK_OFF_WEIGHT;

                //Allocate more memory for probabilities and back off weight for
                //the remaining M-gram levels until M < N. For M==N there is no
                //back-off weights and thus we will store the probabilities just
                //Inside the C container class values.
                for (int idx = 1; idx < (MAX_LEVEL - 1); idx++) {
                    m_mgram_data[idx] = new T_M_Gram_Payload[counts[idx]];
                    memset(m_mgram_data[idx], 0, counts[idx] * sizeof (T_M_Gram_Payload));
                }

                //04) Allocate the word map arrays per level There is N-1 levels to have 
                //as the for M == 0 - the One Grams, we do not need this mappings
                for (int idx = 0; idx < (MAX_LEVEL - 1); idx++) {
                    m_mgram_mapping[idx] = new StorageContainer*[m_word_arr_size];
                    memset(m_mgram_mapping[idx], 0, m_word_arr_size * sizeof (StorageContainer*));
                }
            }

            template<TModelLevel MAX_LEVEL, typename WordIndexType, template<TModelLevel > class StorageFactory, class StorageContainer>
            W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>::~W2CHybridTrie() {
                //Delete the probability and back-off data
                for (TModelLevel idx = 0; idx < (MAX_LEVEL - 1); idx++) {
                    //Delete the prob/back-off arrays per level
                    if (m_mgram_data[idx] != NULL) {
                        delete[] m_mgram_data[idx];
                    }
                }
                //Delete the mapping data
                for (TModelLevel idx = 0; idx < (MAX_LEVEL - 1); idx++) {
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
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CHybridTrie, BasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CHybridTrie, CountingWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CHybridTrie, TOptBasicWordIndex);
            INSTANTIATE_LAYERED_TRIE_TEMPLATES_NAME_TYPE(W2CHybridTrie, TOptCountWordIndex);
        }
    }
}
