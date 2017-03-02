/* 
 * File:   C2DHybridTrie.hpp
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

#ifndef C2DMAPARRAYTRIE_HPP
#define C2DMAPARRAYTRIE_HPP

#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"

#include "layered_trie_base.hpp"

#include "server/lm/dictionaries/hashing_word_index.hpp"
#include "common/utils/containers/greedy_memory_allocator.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::containers::alloc;
using namespace uva::smt::bpbd::server::lm::identifiers;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    /**
                     * This is a hybrid trie implementation inspired by the four other ones:
                     * 
                     * W2COrderedArrayTrie, C2WOrderedArrayTrie,
                     * CtxMultiHashMapTrie, and W2CHybridMemoryTrie
                     * 
                     * It tries to be as much memory efficient as speed efficient. More
                     * specifically we store as much data as possible in an array form in
                     * order to get optimal memory consumption and having short and easily
                     * definable context index. Yet, we use unordered maps for the sake of
                     * speeding up queries, as they allow us to realize (word_id, ctx_id) to ctx_id
                     * in the most efficient manner. The lookup should be just O(1) whereas in
                     * the lookup is O(log(n)), as we need to use binary searches there.
                     */
                    template<typename WordIndexType>
                    class c2d_hybrid_trie : public layered_trie_base<c2d_hybrid_trie<WordIndexType>, WordIndexType, __C2DHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef layered_trie_base<c2d_hybrid_trie<WordIndexType>, WordIndexType, __C2DHybridTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

                        /**
                         * The basic class constructor, accepts memory factors that are the
                         * coefficients used when pre-allocating memory for unordered maps.
                         * 
                         * If a factor is equal to 0.0 then no memory is pre-allocated.
                         * If the factor is equal to 1.0 then there is only as much preallocated
                         * as needed to store the gram entries. The latter is typically not enough
                         * as unordered_map needs more memory for internal administration.
                         * If there is not enough memory pre-allocated then additional allocations
                         * will take place but it does not alway lead to more efficient memory
                         * usage. The observed behavior is that it is better to pre-allocate
                         * a bit more memory beforehand, than needed. This leads to less
                         * memory consumption. Depending on the type of unordered_map
                         * key/value pair types the advised factor values are from 2.0 to 2.6.
                         * Because it can not be optimally determined beforehand, these are made
                         * constructor parameters so that they can be configured by the used.
                         * This breaks encapsulation a bit, exposing the internals, but
                         * there is no other better way, for fine tuning the memory usage.
                         * @param word_index the word index to be used
                         * @param mram_mem_factor The M-Gram memory factor needed for
                         * the greedy allocator for the unordered_map
                         * @param ngram_mem_factor The N-Gram memory factor needed for
                         * the greedy allocator for the unordered_map
                         */
                        explicit c2d_hybrid_trie(WordIndexType & word_index,
                                const float mram_mem_factor = __C2DHybridTrie::UM_M_GRAM_MEMORY_FACTOR,
                                const float ngram_mem_factor = __C2DHybridTrie::UM_N_GRAM_MEMORY_FACTOR);

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
                            const TLongId key = put_32_32_in_64(ctx_id, word_id);

                            //Search for the map for that context id
                            TMGramsMap::const_iterator result = m_m_gram_map_ptrs[level_idx]->find(key);
                            if (result == m_m_gram_map_ptrs[level_idx]->end()) {
                                //There is no data found under this context
                                return false;
                            } else {
                                //Update the context with the found value uf the next context
                                ctx_id = result->second;
                                //The context can always be computed
                                return true;
                            }
                        }

                        /**
                         * Allows to log the information about the instantiated trie type
                         */
                        inline void log_model_type_info() const {
                            LOG_USAGE << "Using the <" << __FILENAME__ << "> model." << END_LOG;
                        }

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
                                m_1_gram_data[word_id] = gram.m_payload;
                            } else {
                                //Register the m-gram in the hash cache
                                this->register_m_gram_cache(gram);

                                //Define the context id variable
                                TLongId ctx_id = UNKNOWN_WORD_ID;
                                //Obtain the m-gram context id
                                __LayeredTrieBase::get_context_id<c2d_hybrid_trie<WordIndexType>, CURR_LEVEL, debug_levels_enum::DEBUG2>(*this, gram, ctx_id);

                                //Obtain the context key and then create a new mapping
                                const TLongId key = put_32_32_in_64(ctx_id, word_id);

                                //Store the payload
                                if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                    m_n_gram_map_ptr->operator[](key) = gram.m_payload.m_prob;
                                } else {
                                    //Get the next context id
                                    const phrase_length level_idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                                    TShortId next_ctx_id = m_M_gram_next_ctx_id[level_idx]++;

                                    //Store the context mapping inside the map
                                    m_m_gram_map_ptrs[level_idx]->operator[](key) = next_ctx_id;

                                    //Return the reference to the piece of memory
                                    m_m_gram_data[level_idx][next_ctx_id] = gram.m_payload;
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
                            query.set_curr_payload(m_1_gram_data[word_id]);
                            
                            LOG_DEBUG << "The uni-gram word id " << SSTR(word_id) << " payload : "
                                    << m_1_gram_data[word_id] << END_LOG;
                        };

                        /**
                         * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                         * @see GenericTrieBase
                         */
                        inline void get_m_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            LOG_DEBUG << "Getting the payload for sub-m-gram : "<< query << END_LOG;

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
                                    LOG_DEBUG << "level_idx: " << SSTR(level_idx) << ", ctx_id: " << ctx_id << END_LOG;
                                    //There is data found under this context
                                    query.set_curr_payload(m_m_gram_data[level_idx][ctx_id]);
                                    LOG_DEBUG << "The payload is retrieved: " << m_m_gram_data[level_idx][ctx_id] << END_LOG;
                                } else {
                                    //The payload could not be found
                                    LOG_DEBUG1 << "Unable to find m-gram data for ctx_id: " << SSTR(ctx_id)
                                            << ", word_id: " << SSTR(word_id) << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                                LOG_DEBUG << "Context ensure status is: " << status_to_string(status) << END_LOG;
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

                                const TLongId key = put_32_32_in_64(ctx_id, word_id);

                                //Search for the map for that context id
                                TNGramsMap::const_iterator result = m_n_gram_map_ptr->find(key);
                                if (result == m_n_gram_map_ptr->end()) {
                                    //The payload could not be found
                                    LOG_DEBUG1 << "Unable to find " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-gram data for ctx_id: "
                                            << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                } else {
                                    //There is data found under this context
                                    query.set_curr_payload(result->second);
                                    LOG_DEBUG << "The payload is retrieved: " << result->second << END_LOG;
                                }
                            }
                        }

                        /**
                         * @see word_index_trie_base
                         */
                        void set_def_unk_word_prob(const prob_weight prob);

                        /**
                         * The basic destructor
                         */
                        virtual ~c2d_hybrid_trie();

                    private:
                        //Stores the pointer to the UNK word payload
                        m_gram_payload * m_unk_data;

                        //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                        const float m_mgram_mem_factor;
                        //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                        const float m_ngram_mem_factor;

                        //Stores the context id counters per M-gram level: 1 < M < N
                        TShortId m_M_gram_next_ctx_id[BASE::NUM_M_GRAM_LEVELS];
                        //Stores the context id counters per M-gram level: 1 < M <= N
                        TShortId m_M_gram_num_ctx_ids[BASE::NUM_M_N_GRAM_LEVELS];

                        //Stores the 1-gram data
                        m_gram_payload * m_1_gram_data;

                        //The type of key,value pairs to be stored in the M Grams map
                        typedef pair< const TLongId, TShortId> TMGramEntry;
                        //The typedef for the M Grams map allocator
                        typedef greedy_memory_allocator< TMGramEntry > TMGramAllocator;
                        //The N Grams map type
                        typedef unordered_map<TLongId, TShortId, std::hash<TLongId>, std::equal_to<TLongId>, TMGramAllocator > TMGramsMap;
                        //The actual data storage for the M Grams for 1 < M < N
                        TMGramAllocator * m_m_gram_alloc_ptrs[BASE::NUM_M_GRAM_LEVELS];
                        //The array of maps map storing M-grams for 1 < M < N
                        TMGramsMap * m_m_gram_map_ptrs[BASE::NUM_M_GRAM_LEVELS];
                        //Stores the M-gram data for the M levels: 1 < M < N
                        //This is a two dimensional array
                        m_gram_payload * m_m_gram_data[BASE::NUM_M_GRAM_LEVELS];

                        //The type of key,value pairs to be stored in the N Grams map
                        typedef pair< const TLongId, prob_weight> TNGramEntry;
                        //The typedef for the N Grams map allocator
                        typedef greedy_memory_allocator< TNGramEntry > TNGramAllocator;
                        //The N Grams map type
                        typedef unordered_map<TLongId, prob_weight, std::hash<TLongId>, std::equal_to<TLongId>, TNGramAllocator > TNGramsMap;
                        //The actual data storage for the N Grams
                        TNGramAllocator * m_n_gram_alloc_ptr;
                        //The map storing the N-Grams, they do not have back-off values
                        TNGramsMap * m_n_gram_map_ptr;

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void preAllocateOGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void preAllocateMGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void preAllocateNGrams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                    };

                    typedef c2d_hybrid_trie< basic_word_index > TC2DHybridTrieBasic;
                    typedef c2d_hybrid_trie< counting_word_index > TC2DHybridTrieCount;
                    typedef c2d_hybrid_trie< basic_optimizing_word_index > TC2DHybridTrieOptBasic;
                    typedef c2d_hybrid_trie< counting_optimizing_word_index > TC2DHybridTrieOptCount;
                    typedef c2d_hybrid_trie< hashing_word_index > TC2DHybridTrieHashing;
                }
            }
        }
    }
}
#endif /* CONTEXTMULTIHASHMAPTRIE_HPP */

