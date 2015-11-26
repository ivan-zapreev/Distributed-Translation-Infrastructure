/* 
 * File:   W2CHybridTrie.hpp
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
#include "HashingWordIndex.hpp"
#include "W2CH_UM_Storage.hpp"

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
            class W2CHybridTrie : public LayeredTrieBase<W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>, MAX_LEVEL, WordIndexType, __W2CHybridTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef LayeredTrieBase<W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>, MAX_LEVEL, WordIndexType, __W2CHybridTrie::DO_BITMAP_HASH_CACHE> BASE;

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit W2CHybridTrie(WordIndexType & word_index);

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M <= N!
                 * @see LayeredTrieBase
                 * 
                 * @param word_id the current word id
                 * @param ctx_id [in] - the previous context id, [out] - the next context id
                 * @param curr_level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw nothing
                 */
                inline bool get_ctx_id(const TModelLevel curr_level, const TShortId word_id, TLongId & ctx_id) const {
                    LOG_DEBUG3 << "Retrieving context level: " << curr_level << ", word_id: "
                            << word_id << ", ctx_id: " << ctx_id << END_LOG;
                    //Retrieve the context data for the given word
                    StorageContainer* ctx_mapping = m_mgram_mapping[curr_level - BASE::MGRAM_IDX_OFFSET][word_id];

                    //Check that the context data is available
                    if (ctx_mapping != NULL) {
                        typename StorageContainer::const_iterator result = ctx_mapping->find(ctx_id);
                        if (result == ctx_mapping->end()) {
                            LOG_DEBUG2 << "Can not find ctx_id: " << SSTR(ctx_id) << " for level: "
                                    << SSTR(curr_level) << ", word_id: " << SSTR(word_id) << END_LOG;
                            return false;
                        } else {
                            LOG_DEBUG2 << "Found next ctx_id: " << SSTR(result->second)
                                    << " for level: " << SSTR(curr_level) << ", word_id: "
                                    << SSTR(word_id) << ", ctx_id: " << SSTR(ctx_id) << END_LOG;

                            ctx_id = result->second;
                            return true;
                        }
                    } else {
                        LOG_DEBUG2 << "No context data for: " << SSTR(curr_level)
                                << ", word_id: " << SSTR(word_id) << END_LOG;
                        return false;
                    }
                }

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
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    const TShortId word_id = gram.get_end_word_id();
                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Store the payload
                        m_mgram_data[0][word_id] = gram.m_payload;
                    } else {
                        //Register the m-gram in the hash cache
                        this->register_m_gram_cache(gram);

                        //Define the context id variable
                        TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                        //Obtain the m-gram context id
                        __LayeredTrieBase::get_context_id<W2CHybridTrie<MAX_LEVEL, WordIndexType, StorageFactory, StorageContainer>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                        //Store the payload
                        if (CURR_LEVEL == MAX_LEVEL) {
                            StorageContainer*& ctx_mapping = m_mgram_mapping[BASE::N_GRAM_IDX_IN_M_N_ARR][word_id];
                            if (ctx_mapping == NULL) {
                                ctx_mapping = m_storage_factory->create(MAX_LEVEL);
                                LOG_DEBUG3 << "Allocating storage for level " << SSTR(MAX_LEVEL) << ", word_id " << SSTR(word_id) << END_LOG;
                            }

                            LOG_DEBUG3 << "Returning reference to prob., level: " << SSTR(MAX_LEVEL) << ", word_id "
                                    << SSTR(word_id) << ", ctx_id " << SSTR(ctx_id) << END_LOG;
                            //WARNING: We cast to (TLogProbBackOff &) as we misuse the mapping by storing the probability value there!
                            reinterpret_cast<TLogProbBackOff&> (ctx_mapping->operator[](ctx_id)) = gram.m_payload.m_prob;
                        } else {
                            const TModelLevel idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                            //Get the word mapping first
                            StorageContainer*& ctx_mapping = m_mgram_mapping[idx][word_id];

                            //If the mappings is not there yet for the contexts then create it
                            if (ctx_mapping == NULL) {
                                ctx_mapping = m_storage_factory->create(CURR_LEVEL);
                                LOG_DEBUG3 << "A new ACtxToPBStorage container is allocated for level " << SSTR(CURR_LEVEL) << END_LOG;
                            }

                            //Add the new element to the context mapping
                            TShortId & nextCtxId = ctx_mapping->operator[](ctx_id);
                            nextCtxId = next_ctx_id[idx]++;

                            //Return the reference to it
                            m_mgram_data[CURR_LEVEL - 1][nextCtxId] = gram.m_payload;
                        }
                    }
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success
                 * @see GenericTrieBase
                 */
                inline void get_unigram_payload(typename BASE::T_Query_Exec_Data & query) const {
                    //Get the word index for convenience
                    const TModelLevel & word_idx = query.m_begin_word_idx;

                    LOG_DEBUG << "Getting the payload for sub-uni-gram : [" << SSTR(word_idx)
                            << "," << SSTR(word_idx) << "]" << END_LOG;

                    //The data is always present.
                    query.m_payloads[word_idx][word_idx] = &m_mgram_data[0][query.m_gram[word_idx]];
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                inline void get_m_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    LOG_DEBUG << "Getting the payload for sub-m-gram : [" << SSTR(query.m_begin_word_idx)
                            << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;

                    //First ensure the context of the given sub-m-gram
                    LAYERED_BASE_ENSURE_CONTEXT(query, status);

                    //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Store the shorthand for the context and end word id
                        TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];
                        const TShortId & word_id = query.m_gram[query.m_end_word_idx];

                        //Compute the distance between words
                        const TModelLevel curr_level = (query.m_end_word_idx - query.m_begin_word_idx) + 1;
                        LOG_DEBUG << "curr_level: " << SSTR(curr_level) << ", ctx_id: " << ctx_id << ", m_end_word_idx: "
                                << SSTR(query.m_end_word_idx) << ", end word id: " << word_id << END_LOG;

                        //Get the next context id
                        if (get_ctx_id(curr_level, word_id, ctx_id)) {
                            LOG_DEBUG << "ctx_id: " << ctx_id << END_LOG;
                            const TModelLevel level_idx = curr_level - 1;
                            //There is data found under this context
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &m_mgram_data[level_idx][ctx_id];
                            LOG_DEBUG << "The payload is retrieved: " << (string) m_mgram_data[level_idx][ctx_id] << END_LOG;
                        } else {
                            //The payload could not be found
                            LOG_DEBUG1 << "Unable to find m-gram data for ctx_id: " << SSTR(ctx_id)
                                    << ", word_id: " << SSTR(word_id) << END_LOG;
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        }
                    }
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==n
                 * @see GenericTrieBase
                 */
                inline void get_n_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //First ensure the context of the given sub-m-gram
                    LAYERED_BASE_ENSURE_CONTEXT(query, status);

                    //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Store the shorthand for the context and end word id
                        TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];
                        const TShortId & word_id = query.m_gram[query.m_end_word_idx];

                        //Try to find the word mapping first
                        StorageContainer*& ctx_mapping = m_mgram_mapping[BASE::N_GRAM_IDX_IN_M_N_ARR][word_id];

                        //If the mapping is present the search further, otherwise return false
                        if (ctx_mapping != NULL) {
                            typename StorageContainer::const_iterator result = ctx_mapping->find(ctx_id);
                            if (result == ctx_mapping->end()) {
                                //The data could not be found
                                status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                            } else {
                                //The data could be found
                                query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &result->second;
                                LOG_DEBUG << "The payload is retrieved: " << result->second << END_LOG;
                            }
                        } else {
                            //The payload could not be found
                            LOG_DEBUG1 << "Unable to find " << SSTR(MAX_LEVEL) << "-gram data for ctx_id: "
                                    << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        }
                    }
                }

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
            typedef W2CHybridTrie<M_GRAM_LEVEL_MAX, HashingWordIndex > TW2CHybridTrieHashing;
        }
    }
}

#endif	/* HYBRIDMEMORYTRIE_HPP */

