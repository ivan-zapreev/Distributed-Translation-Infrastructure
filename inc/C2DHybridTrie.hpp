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
#define	C2DMAPARRAYTRIE_HPP

#include <utility>        // std::pair, std::make_pair
#include <unordered_map>  // std::unordered_map

#include "Globals.hpp"
#include "Logger.hpp"

#include "LayeredTrieBase.hpp"

#include "HashingWordIndex.hpp"
#include "GreedyMemoryAllocator.hpp"
#include "HashingUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::hashing;
using namespace uva::smt::logging;
using namespace uva::smt::tries::alloc;

namespace uva {
    namespace smt {
        namespace tries {

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
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class C2DHybridTrie : public LayeredTrieBase<C2DHybridTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DHybridTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef LayeredTrieBase<C2DHybridTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __C2DHybridTrie::DO_BITMAP_HASH_CACHE> BASE;

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
                 * @param _pWordIndex the word index to be used
                 * @param _oGramMemFactor The One-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 * @param _mGramMemFactor The M-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 * @param _nGramMemFactor The N-Gram memory factor needed for
                 * the greedy allocator for the unordered_map
                 */
                explicit C2DHybridTrie(WordIndexType & word_index,
                        const float mram_mem_factor = __C2DHybridTrie::UM_M_GRAM_MEMORY_FACTOR,
                        const float ngram_mem_factor = __C2DHybridTrie::UM_N_GRAM_MEMORY_FACTOR);

                /**
                 * Computes the M-Gram context using the previous context and the current word id
                 * @see LayeredTrieBese
                 */
                inline bool get_ctx_id(const TModelLevel level_idx, const TShortId word_id, TLongId & ctx_id) const {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

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
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Register the m-gram in the hash cache
                        this->register_m_gram_cache(gram);

                        //Define the context id variable
                        TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                        //Obtain the m-gram context id
                        __LayeredTrieBase::get_context_id<C2DHybridTrie<MAX_LEVEL, WordIndexType>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                        //Obtain the context key and then create a new mapping
                        const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

                        //Store the payload
                        if (CURR_LEVEL == MAX_LEVEL) {
                            m_n_gram_map_ptr->operator[](key) = gram.m_payload.m_prob;
                        } else {
                            //Get the next context id
                            const TModelLevel idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                            TShortId next_ctx_id = m_M_gram_next_ctx_id[idx]++;

                            //Store the context mapping inside the map
                            m_m_gram_map_ptrs[idx]->operator[](key) = next_ctx_id;

                            //Return the reference to the piece of memory
                            m_m_gram_data[idx][next_ctx_id] = gram.m_payload;
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
                    query.m_payloads[word_idx][word_idx] = &m_1_gram_data[query.m_gram[word_idx]];
                };

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * @see GenericTrieBase
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
                        const TModelLevel & curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];
                        LOG_DEBUG << "curr_level: " << SSTR(curr_level) << ", ctx_id: " << ctx_id << ", m_end_word_idx: "
                                << SSTR(query.m_end_word_idx) << ", end word id: " << word_id << END_LOG;

                        //Get the next context id
                        const TModelLevel & level_idx = CURR_LEVEL_MIN_2_MAP[query.m_begin_word_idx][query.m_end_word_idx];
                        if (get_ctx_id(level_idx, word_id, ctx_id)) {
                            LOG_DEBUG << "level_idx: " << SSTR(level_idx) << ", ctx_id: " << ctx_id << END_LOG;
                            //There is data found under this context
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &m_m_gram_data[level_idx][ctx_id];
                            LOG_DEBUG << "The payload is retrieved: " << (string) m_m_gram_data[level_idx][ctx_id] << END_LOG;
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
                inline void get_n_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //First ensure the context of the given sub-m-gram
                    LAYERED_BASE_ENSURE_CONTEXT(query, status);

                    //If the context is successfully ensured, then move on to the m-gram and try to obtain its payload
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Store the shorthand for the context and end word id
                        TLongId & ctx_id = query.m_last_ctx_ids[query.m_begin_word_idx];
                        const TShortId & word_id = query.m_gram[query.m_end_word_idx];

                        const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

                        //Search for the map for that context id
                        TNGramsMap::const_iterator result = m_n_gram_map_ptr->find(key);
                        if (result == m_n_gram_map_ptr->end()) {
                            //The payload could not be found
                            LOG_DEBUG1 << "Unable to find " << SSTR(MAX_LEVEL) << "-gram data for ctx_id: "
                                    << SSTR(ctx_id) << ", word_id: " << SSTR(word_id) << END_LOG;
                            status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                        } else {
                            //There is data found under this context
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &result->second;
                            LOG_DEBUG << "The payload is retrieved: " << result->second << END_LOG;
                        }
                    }
                }

                /**
                 * The basic destructor
                 */
                virtual ~C2DHybridTrie();

            private:

                //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                const float m_mgram_mem_factor;
                //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                const float m_ngram_mem_factor;

                //Stores the context id counters per M-gram level: 1 < M < N
                TShortId m_M_gram_next_ctx_id[BASE::NUM_M_GRAM_LEVELS];
                //Stores the context id counters per M-gram level: 1 < M <= N
                TShortId m_M_gram_num_ctx_ids[BASE::NUM_M_N_GRAM_LEVELS];

                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //The type of key,value pairs to be stored in the M Grams map
                typedef pair< const TLongId, TShortId> TMGramEntry;
                //The typedef for the M Grams map allocator
                typedef GreedyMemoryAllocator< TMGramEntry > TMGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, TShortId, std::hash<TLongId>, std::equal_to<TLongId>, TMGramAllocator > TMGramsMap;
                //The actual data storage for the M Grams for 1 < M < N
                TMGramAllocator * m_m_gram_alloc_ptrs[BASE::NUM_M_GRAM_LEVELS];
                //The array of maps map storing M-grams for 1 < M < N
                TMGramsMap * m_m_gram_map_ptrs[BASE::NUM_M_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                T_M_Gram_Payload * m_m_gram_data[BASE::NUM_M_GRAM_LEVELS];

                //The type of key,value pairs to be stored in the N Grams map
                typedef pair< const TLongId, TLogProbBackOff> TNGramEntry;
                //The typedef for the N Grams map allocator
                typedef GreedyMemoryAllocator< TNGramEntry > TNGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, TLogProbBackOff, std::hash<TLongId>, std::equal_to<TLongId>, TNGramAllocator > TNGramsMap;
                //The actual data storage for the N Grams
                TNGramAllocator * m_n_gram_alloc_ptr;
                //The map storing the N-Grams, they do not have back-off values
                TNGramsMap * m_n_gram_map_ptr;

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateOGrams(const size_t counts[MAX_LEVEL]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateMGrams(const size_t counts[MAX_LEVEL]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateNGrams(const size_t counts[MAX_LEVEL]);

            };

            typedef C2DHybridTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TC2DHybridTrieBasic;
            typedef C2DHybridTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TC2DHybridTrieCount;
            typedef C2DHybridTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TC2DHybridTrieOptBasic;
            typedef C2DHybridTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TC2DHybridTrieOptCount;
            typedef C2DHybridTrie<M_GRAM_LEVEL_MAX, HashingWordIndex > TC2DHybridTrieHashing;
        }
    }
}
#endif	/* CONTEXTMULTIHASHMAPTRIE_HPP */

