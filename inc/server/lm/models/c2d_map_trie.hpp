/* 
 * File:   C2DMapTrie.hpp
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

#ifndef C2DHASHMAPTRIE_HPP
#define C2DHASHMAPTRIE_HPP

/**
 * We actually have several choices:
 * 
 * Continue to use <ext/hash_map.h> and use -Wno-deprecated to stop the warning
 * 
 * Use <tr1/unordered_map> and std::tr1::unordered_map
 * 
 * Use <unordered_map> and std::unordered_map and -std=c++0x
 * 
 * We will need to test which one runs better, it is an unordered_map for now.
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 */
#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"

#include "layered_trie_base.hpp"

#include "common/utils/containers/greedy_memory_allocator.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"

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
                     * This is a HashMpa based ATrie interface implementation class.
                     * Note 1: This implementation uses the unsigned long for the hashes it is not optimal
                     * Note 2: the unordered_map might be not as efficient as a hash_map with respect to memory usage but it is supposed to be faster
                     * 
                     * This implementation is chosen because it resembles the ordered array implementation from:
                     *      "Faster and Smaller N -Gram Language Models"
                     *      Adam Pauls Dan Klein
                     *      Computer Science Division
                     *      University of California, Berkeley
                     * 
                     * and unordered_maps showed good performance in:
                     *      "Efficient in-memory data structures for n-grams indexing"
                     *       Daniel Robenek, Jan Platoˇs, and V ́aclav Sn ́aˇsel
                     *       Department of Computer Science, FEI, VSB – Technical University of Ostrava
                     *       17. listopadu 15, 708 33, Ostrava-Poruba, Czech Republic
                     *       {daniel.robenek.st, jan.platos, vaclav.snasel}@vsb.cz
                     * 
                     */
                    template<typename WordIndexType>
                    class C2DMapTrie : public LayeredTrieBase<C2DMapTrie<WordIndexType>, WordIndexType, __C2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef LayeredTrieBase<C2DMapTrie<WordIndexType>, WordIndexType, __C2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;

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
                         * 
                         * @param word_index the word index to be used
                         * @param mgram_mem_factor The M-Gram memory factor needed for
                         * the greedy allocator for the unordered_map
                         * @param ngram_mem_factor The N-Gram memory factor needed for
                         * the greedy allocator for the unordered_map
                         */
                        explicit C2DMapTrie(WordIndexType & word_index,
                                const float mgram_mem_factor = __C2DMapTrie::UM_M_GRAM_MEMORY_FACTOR,
                                const float ngram_mem_factor = __C2DMapTrie::UM_N_GRAM_MEMORY_FACTOR);

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
                            //Use the Szudzik algorithm as it outperforms Cantor
                            ctx_id = szudzik(word_id, ctx_id);
                            //The context can always be computed
                            return true;
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
                                __LayeredTrieBase::get_context_id<C2DMapTrie<WordIndexType>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                                //Obtain this m-gram id
                                (void) get_ctx_id(CURR_LEVEL - BASE::MGRAM_IDX_OFFSET, word_id, ctx_id);

                                //Store the payload
                                if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                    m_n_gram_map_ptr->operator[](ctx_id) = gram.m_payload.m_prob;
                                } else {
                                    m_m_gram_map_ptrs[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET]->operator[](ctx_id) = gram.m_payload;
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
                            query.set_curr_payload(&m_1_gram_data[word_id]);
                            
                            LOG_DEBUG << "The uni-gram word id " << SSTR(word_id) << " payload : "
                                    << (string) m_1_gram_data[word_id] << END_LOG;
                        };

                        /**
                         * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                         * For more details @see LayeredTrieBase
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
                                    TMGramsMap::const_iterator result = m_m_gram_map_ptrs[level_idx]->find(ctx_id);
                                    if (result == m_m_gram_map_ptrs[level_idx]->end()) {
                                        //The payload could not be found
                                        LOG_DEBUG1 << "Unable to find m-gram data for ctx_id: " << SSTR(ctx_id)
                                                << ", word_id: " << SSTR(word_id) << END_LOG;
                                        status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                    } else {
                                        //There is data found under this context
                                        query.set_curr_payload(&result->second);
                                        LOG_DEBUG << "The payload is retrieved: " << (string) result->second << END_LOG;
                                    }
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
                                TLongId & ctx_id = query.get_curr_ctx_ref();
                                const TShortId & word_id = query.get_curr_end_word_id();

                                LOG_DEBUG << "ctx_id: " << ctx_id << ", m_end_word_idx: " << SSTR(query.m_curr_end_word_idx)
                                        << ", end word id: " << word_id << END_LOG;
                                //Get the next context id
                                const phrase_length & level_idx = query.get_curr_level_m2();
                                if (get_ctx_id(level_idx, word_id, ctx_id)) {
                                    LOG_DEBUG << "ctx_id: " << ctx_id << END_LOG;
                                    TNGramsMap::const_iterator result = m_n_gram_map_ptr->find(ctx_id);
                                    if (result == m_n_gram_map_ptr->end()) {
                                        //The payload could not be found
                                        LOG_DEBUG1 << "Unable to find " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-gram data for ctx_id: "
                                                << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                                        status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                    } else {
                                        //There is data found under this context
                                        query.set_curr_payload(&result->second);
                                        LOG_DEBUG << "The payload is retrieved: " << result->second << END_LOG;
                                    }
                                } else {
                                    //The payload could not be found
                                    LOG_DEBUG << "The payload id could not be found!" << END_LOG;
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                                LOG_DEBUG << "Context ensure status is: " << status_to_string(status) << END_LOG;
                            }
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~C2DMapTrie();

                    private:
                        //Stores the pointer to the UNK word payload
                        m_gram_payload * m_unk_data;

                        //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                        const float m_mgram_mem_factor;
                        //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                        const float m_ngram_mem_factor;

                        //Stores the 1-gram data
                        m_gram_payload * m_1_gram_data;

                        //The type of key,value pairs to be stored in the M Grams map
                        typedef pair< const TLongId, m_gram_payload> TMGramEntry;
                        //The typedef for the M Grams map allocator
                        typedef GreedyMemoryAllocator< TMGramEntry > TMGramAllocator;
                        //The N Grams map type
                        typedef unordered_map<TLongId, m_gram_payload, std::hash<TLongId>, std::equal_to<TLongId>, TMGramAllocator > TMGramsMap;
                        //The actual data storage for the M Grams for 1 < M < N
                        TMGramAllocator * m_m_gram_alloc_ptrs[LM_M_GRAM_LEVEL_MAX - BASE::MGRAM_IDX_OFFSET];
                        //The array of maps map storing M-grams for 1 < M < N
                        TMGramsMap * m_m_gram_map_ptrs[LM_M_GRAM_LEVEL_MAX - BASE::MGRAM_IDX_OFFSET];

                        //The type of key,value pairs to be stored in the N Grams map
                        typedef pair< const TLongId, prob_weight> TNGramEntry;
                        //The typedef for the N Grams map allocator
                        typedef GreedyMemoryAllocator< TNGramEntry > TNGramAllocator;
                        //The N Grams map type
                        typedef unordered_map<TLongId, prob_weight, std::hash<TLongId>, std::equal_to<TLongId>, TNGramAllocator > TNGramsMap;
                        //The actual data storage for the N Grams
                        TNGramAllocator * m_n_gram_alloc_ptr;
                        //The map storing the N-Grams, they do not have back-off values
                        TNGramsMap * m_n_gram_map_ptr;

                        //The structure for storing the hash key values statistics
                        pair<TLongId, TLongId> m_hash_sizes[LM_M_GRAM_LEVEL_MAX];

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void pre_allocate_1_grams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void pre_allocate_m_grams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * This method must used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @param counts the counts for the number of elements of each gram level
                         */
                        void pre_allocate_n_grams(const size_t counts[LM_M_GRAM_LEVEL_MAX]);
                    };

                    typedef C2DMapTrie<basic_word_index > TC2DMapTrieBasic;
                    typedef C2DMapTrie<counting_word_index > TC2DMapTrieCount;
                    typedef C2DMapTrie<hashing_word_index > TC2DMapTrieHashing;
                    typedef C2DMapTrie<basic_optimizing_word_index > TC2DMapTrieOptBasic;
                    typedef C2DMapTrie<counting_optimizing_word_index > TC2DMapTrieOptCount;
                }
            }
        }
    }
}
#endif /* CONTEXTMULTIHASHMAPTRIE_HPP */

