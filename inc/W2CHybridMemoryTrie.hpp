/* 
 * File:   W2CHybridMemoryTrie.hpp
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

#ifndef W2CHYBRIDMEMORYTRIE_HPP
#define	W2CHYBRIDMEMORYTRIE_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"

#include "LayeredTrieBase.hpp"

#include "AWordIndex.hpp"
#include "W2CHybridMemoryTrieStorage.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the hybrid memory trie implementation class. It has three template parameters.
             * @param MAX_LEVEL the maximum number of levelns in the trie.
             * @param StorageFactory the factory to create storage containers
             * @param StorageContainer the storage container type that is created by the factory
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType, template<TModelLevel > class StorageFactory = W2CH_UM_StorageFactory, class StorageContainer = W2CH_UM_Storage>
            class W2CHybridTrie : public LayeredTrieBase<MAX_LEVEL, WordIndexType> {
            public:
                typedef LayeredTrieBase<MAX_LEVEL, WordIndexType> BASE;

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit W2CHybridTrie(WordIndexType & word_index);

                /**
                 * @see GenericTrieBase
                 */
                constexpr static inline bool needs_bitmap_hash_cache() {
                    return __W2CHybridTrie::DO_BITMAP_HASH_CACHE;
                }

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M <= N!
                 * @see LayeredTrieBase
                 * 
                 * @param wordId the current word id
                 * @param ctxId [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw nothing
                 */
                template<TModelLevel level>
                bool get_ctx_id(const TShortId wordId, TLongId & ctxId) const;

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see LayeredTrieBase
                 */
                virtual void pre_allocate(const size_t counts[MAX_LEVEL]);

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                T_M_Gram_Payload & make_1_gram_data_ref(const TShortId wordId);

                /**
                 * Allows to retrieve the payload for the One gram with the given Id.
                 * @see LayeredTrieBase
                 */
                inline bool get_1_gram_payload(const TShortId wordId, T_M_Gram_Payload &payload) const {
                    //The data is always present.
                    payload = m_mgram_data[0][wordId];
                    return true;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel level>
                T_M_Gram_Payload& make_m_gram_data_ref(const TShortId wordId, const TLongId ctxId);

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end wordId and ctxId.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_m_gram_payload(const TShortId wordId, TLongId ctxId,
                        T_M_Gram_Payload &payload) const {
                    //Get the context id, note we use short ids here!
                    if (get_ctx_id<CURR_LEVEL>(wordId, ctxId)) {
                        //Return the data by the context
                        payload = m_mgram_data[CURR_LEVEL - 1][ctxId];
                        return true;
                    } else {
                        return false;
                    }
                }

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * For more details @see LayeredTrieBase
                 */
                TLogProbBackOff& make_n_gram_data_ref(const TShortId wordId, const TLongId ctxId);

                /**
                 * Allows to retrieve the payload for the N gram defined by the
                 * end_word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                inline bool get_n_gram_payload(const TShortId end_word_id, TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    //Try to find the word mapping first
                    StorageContainer*& ctx_mapping = m_mgram_mapping[BASE::N_GRAM_IDX_IN_M_N_ARR][end_word_id];

                    //If the mapping is present the search further, otherwise return false
                    if (ctx_mapping != NULL) {
                        typename StorageContainer::const_iterator result = ctx_mapping->find(ctx_id);
                        if (result == ctx_mapping->end()) {
                            //The data could not be found
                            return false;
                        } else {
                            //The data could be found
                            LOG_DEBUG1 << "Found the probability value: " << result->second << ", end_word_id: "
                                    << SSTR(end_word_id) << ", ctx_id: " << SSTR(ctx_id) << END_LOG;
                            //WARNING: We cast to (TLogProbBackOff &) as we misuse the mapping by storing the probability value there!
                            payload.prob = (TLogProbBackOff &) result->second;
                            return true;
                        }
                    } else {
                        LOG_DEBUG1 << "There are no elements @ level: " << SSTR(MAX_LEVEL)
                                << " for wordId: " << SSTR(end_word_id) << "!" << END_LOG;
                        return false;
                    }
                }

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    payload = m_mgram_data[0][WordIndexType::UNKNOWN_WORD_ID];
                };

                /**
                 * The basic destructor
                 */
                virtual ~W2CHybridTrie();

            private:

                //Stores the number of words
                size_t m_word_arr_size;

                //The factory to produce the storage containers
                StorageFactory<MAX_LEVEL> * m_storage_factory;

                //M-Gram data for 1 <= M < N. This is a 2D array storing
                //For each M-Gram level M an array of prob-back_off values
                // m_mgram_data[M][0] - probability/back-off pair for the given M-gram
                // m_mgram_data[M][1] --//--
                // ...
                // m_mgram_data[M][#M-Grams - 1] --//--
                // m_mgram_data[M][#M-Grams] --//--
                T_M_Gram_Payload * m_mgram_data[MAX_LEVEL - 1];

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
                StorageContainer** m_mgram_mapping[MAX_LEVEL - 1];

                //Will store the next context index counters per M-gram level
                //for 1 < M < N.
                const static TModelLevel NUM_IDX_COUNTERS = MAX_LEVEL - 2;
                TShortId next_ctx_id[NUM_IDX_COUNTERS];

            };

            typedef W2CHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TW2CHybridTrieBasic;
            typedef W2CHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TW2CHybridTrieCount;
            typedef W2CHybridTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TW2CHybridTrieOptBasic;
            typedef W2CHybridTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TW2CHybridTrieOptCount;
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

