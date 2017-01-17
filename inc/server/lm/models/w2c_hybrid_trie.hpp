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
#define W2CHYBRIDMEMORYTRIE_HPP

#include <string>   // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"

#include "layered_trie_base.hpp"

#include "server/lm/dictionaries/aword_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"
#include "server/lm/models/w2ch_um_storage.hpp"

using namespace std;
using namespace uva::smt::bpbd::server::lm::dictionary;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This is the hybrid memory trie implementation class. It has three template parameters.
                     * @param M_GRAM_LEVEL_MAX the maximum number of levelns in the trie.
                     * @param StorageFactory the factory to create storage containers
                     * @param StorageContainer the storage container type that is created by the factory
                     */
                    template<typename WordIndexType, template<phrase_length > class StorageFactory = W2CH_UM_StorageFactory, class StorageContainer = W2CH_UM_Storage>
                    class w2c_hybrid_trie : public layered_trie_base<w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>, WordIndexType, __W2CHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef layered_trie_base<w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>, WordIndexType, __W2CHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

                        /**
                         * The basic constructor
                         * @param p_word_index the word index (dictionary) container
                         */
                        explicit w2c_hybrid_trie(WordIndexType & word_index);

                        /**
                         * Allows to retrieve the unknown target word log probability penalty 
                         * @return the target source word log probability penalty
                         */
                        inline float get_unk_word_prob() const {
                            return m_unk_data->m_prob;
                        }

                        /**
                         * Computes the M-Gram context using the previous context and the current word id
                         * @see LayeredTrieBese
                         */
                        inline bool get_ctx_id(const phrase_length level_idx, const TShortId word_id, TLongId & ctx_id) const {
                            //Compute back the current level pure for debug purposes.
                            const phrase_length curr_level = level_idx + BASE::MGRAM_IDX_OFFSET;
                            LOG_DEBUG3 << "Retrieving context level: " << curr_level << ", word_id: "
                                    << word_id << ", ctx_id: " << ctx_id << END_LOG;

                            //Retrieve the context data for the given word
                            StorageContainer* ctx_mapping = m_mgram_mapping[level_idx][word_id];

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
                        inline void log_model_type_info() const {
                            LOG_USAGE << "Using the <" << __FILENAME__ << "> model." << END_LOG;
                        }

                        /**
                         * @see word_index_trie_base
                         */
                        void set_def_unk_word_prob(const prob_weight prob);

                        /**
                         * This method can be used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * For more details @see LayeredTrieBase
                         */
                        virtual void pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * Allows to retrieve the data storage structure for the M gram
                         * with the given M-gram level Id. M-gram context and last word Id.
                         * If the storage structure does not exist, return a new one.
                         * For more details @see LayeredTrieBase
                         */
                        template<phrase_length CURR_LEVEL>
                        inline void add_m_gram(const model_m_gram & gram) {
                            const TShortId word_id = gram.get_last_word_id();
                            if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                                //Store the payload
                                m_mgram_data[0][word_id] = gram.m_payload;
                            } else {
                                //Register the m-gram in the hash cache
                                this->register_m_gram_cache(gram);

                                //Define the context id variable
                                TLongId ctx_id = UNKNOWN_WORD_ID;
                                //Obtain the m-gram context id
                                __LayeredTrieBase::get_context_id<w2c_hybrid_trie<WordIndexType, StorageFactory, StorageContainer>, CURR_LEVEL, debug_levels_enum::DEBUG2>(*this, gram, ctx_id);

                                //Store the payload
                                if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                    StorageContainer*& ctx_mapping = m_mgram_mapping[BASE::N_GRAM_IDX_IN_M_N_ARR][word_id];
                                    if (ctx_mapping == NULL) {
                                        ctx_mapping = m_storage_factory->create(LM_M_GRAM_LEVEL_MAX);
                                        LOG_DEBUG3 << "Allocating storage for level " << SSTR(LM_M_GRAM_LEVEL_MAX) << ", word_id " << SSTR(word_id) << END_LOG;
                                    }

                                    LOG_DEBUG3 << "Returning reference to prob., level: " << SSTR(LM_M_GRAM_LEVEL_MAX) << ", word_id "
                                            << SSTR(word_id) << ", ctx_id " << SSTR(ctx_id) << END_LOG;
                                    //WARNING: We cast to (TLogProbBackOff &) as we misuse the mapping by storing the probability value there!
                                    reinterpret_cast<prob_weight&> (ctx_mapping->operator[](ctx_id)) = gram.m_payload.m_prob;
                                } else {
                                    const phrase_length idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

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
                        inline void get_unigram_payload(m_gram_query & query) const {
                            //Get the uni-gram word id
                            const word_uid word_id = query.get_curr_uni_gram_word_id();

                            //The data is always present.
                            query.set_curr_payload(m_mgram_data[0][word_id]);
                            
                            LOG_DEBUG << "The uni-gram word id " << SSTR(word_id) << " payload : "
                                    << m_mgram_data[0][word_id] << END_LOG;
                        };

                        /**
                         * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                         * For more details @see LayeredTrieBase
                         */
                        inline void get_m_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            LOG_DEBUG << "Getting the payload for sub-m-gram: " << query << END_LOG;

                            //First ensure the context of the given sub-m-gram
                            LAYERED_BASE_ENSURE_CONTEXT(query, status);

                            //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                //Store the shorthand for the context and end word id
                                TLongId & ctx_id = query.get_curr_ctx_ref();
                                const TShortId & word_id = query.get_curr_end_word_id();

                                //Compute the distance between words
                                const phrase_length & curr_level =query.get_curr_level();
                                LOG_DEBUG << "curr_level: " << SSTR(curr_level) << ", ctx_id: " << ctx_id << ", m_end_word_idx: "
                                        << SSTR(query.m_curr_end_word_idx) << ", end word id: " << word_id << END_LOG;

                                //Get the next context id
                                const phrase_length & level_idx = query.get_curr_level_m2();
                                if (get_ctx_id(level_idx, word_id, ctx_id)) {
                                    LOG_DEBUG << "ctx_id: " << ctx_id << END_LOG;
                                    const phrase_length & idx = query.get_curr_level_m1();
                                    //There is data found under this context
                                    query.set_curr_payload(m_mgram_data[idx][ctx_id]);
                                    LOG_DEBUG << "The payload is retrieved: " << m_mgram_data[idx][ctx_id] << END_LOG;
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
                        inline void get_n_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            //First ensure the context of the given sub-m-gram
                            LAYERED_BASE_ENSURE_CONTEXT(query, status);

                            //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                //Store the shorthand for the context and end word id
                                const TLongId & ctx_id = query.get_curr_ctx_ref();
                                const TShortId & word_id = query.get_curr_end_word_id();

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
                                        query.set_curr_payload(result->second);
                                        LOG_DEBUG << "The payload is retrieved: " << result->second << END_LOG;
                                    }
                                } else {
                                    //The payload could not be found
                                    LOG_DEBUG1 << "Unable to find " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-gram data for ctx_id: "
                                            << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                            }
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~w2c_hybrid_trie();

                    private:
                        //Stores the pointer to the UNK word payload
                        m_gram_payload * m_unk_data;

                        //Stores the number of words
                        size_t m_word_arr_size;

                        //The factory to produce the storage containers
                        StorageFactory<LM_M_GRAM_LEVEL_MAX> * m_storage_factory;

                        //M-Gram data for 1 <= M < N. This is a 2D array storing
                        //For each M-Gram level M an array of prob-back_off values
                        // m_mgram_data[M][0] - probability/back-off pair for the given M-gram
                        // m_mgram_data[M][1] --//--
                        // ...
                        // m_mgram_data[M][#M-Grams - 1] --//--
                        // m_mgram_data[M][#M-Grams] --//--
                        m_gram_payload * m_mgram_data[LM_M_GRAM_LEVEL_MAX - 1];

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
                        StorageContainer** m_mgram_mapping[LM_M_GRAM_LEVEL_MAX - 1];

                        //Will store the next context index counters per M-gram level
                        //for 1 < M < N.
                        const static phrase_length NUM_IDX_COUNTERS = LM_M_GRAM_LEVEL_MAX - 2;
                        TShortId next_ctx_id[NUM_IDX_COUNTERS];
                    };

                    typedef w2c_hybrid_trie<basic_word_index> TW2CHybridTrieBasic;
                    typedef w2c_hybrid_trie<counting_word_index> TW2CHybridTrieCount;
                    typedef w2c_hybrid_trie<basic_optimizing_word_index> TW2CHybridTrieOptBasic;
                    typedef w2c_hybrid_trie<counting_optimizing_word_index> TW2CHybridTrieOptCount;
                    typedef w2c_hybrid_trie<hashing_word_index> TW2CHybridTrieHashing;
                }
            }
        }
    }
}

#endif /* HYBRIDMEMORYTRIE_HPP */

