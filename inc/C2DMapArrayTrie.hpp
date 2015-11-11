/* 
 * File:   C2DMapArrayTrie.hpp
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
            class C2DHybridTrie : public LayeredTrieBase<MAX_LEVEL, WordIndexType, __C2DHybridTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef LayeredTrieBase<MAX_LEVEL, WordIndexType, __C2DHybridTrie::DO_BITMAP_HASH_CACHE> BASE;

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
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M < N!
                 * @see LayeredTrieBase
                 * 
                 * @param word_id the current word id
                 * @param ctx_id [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M, default UNDEF_NGRAM_LEVEL
                 * @return the resulting context
                 * @throw nothing
                 */
                template<TModelLevel CURR_LEVEL>
                inline bool get_ctx_id(const TShortId word_id, TLongId & ctx_id) const {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

                    //Search for the map for that context id
                    const TModelLevel idx = CURR_LEVEL - BASE::MGRAM_IDX_OFFSET;
                    TMGramsMap::const_iterator result = pMGramMap[idx]->find(key);
                    if (result == pMGramMap[idx]->end()) {
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
                 * Allows to retrieve the payload for the One gram with the given Id.
                 * @see LayeredTrieBase
                 */
                inline void get_1_gram_payload(const TShortId word_id, T_M_Gram_Payload &payload) const {
                    //The data is always present.
                    payload = m_1_gram_data[word_id];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    //Register the m-gram in the hash cache
                    this->template register_m_gram_cache<CURR_LEVEL>(gram);
                    
                    const TShortId word_id = gram.get_end_word_id();
                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Store the payload
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Define the context id variable
                        TLongId ctx_id = WordIndexType::UNKNOWN_WORD_ID;
                        //Obtain the m-gram context id
                        __LayeredTrieBase::get_context_id<C2DHybridTrie<MAX_LEVEL, WordIndexType>, CURR_LEVEL, DebugLevelsEnum::DEBUG2>(*this, gram, ctx_id);

                        //Obtain the context key and then create a new mapping
                        const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

                        //Store the payload
                        if (CURR_LEVEL == MAX_LEVEL) {
                            pNGramMap->operator[](key) = gram.m_payload.prob;
                        } else {
                            //Get the next context id
                            const TModelLevel idx = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                            TShortId next_ctx_id = m_M_gram_next_ctx_id[idx]++;

                            //Store the context mapping inside the map
                            pMGramMap[idx]->operator[](key) = next_ctx_id;

                            //Return the reference to the piece of memory
                            m_M_gram_data[idx][next_ctx_id] = gram.m_payload;
                        }
                    }
                }

                /**
                 * Allows to retrieve the payload for the M-gram defined by the end word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline GPR_Enum get_m_gram_payload(const TShortId word_id, TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    //Get the next context id
                    if (get_ctx_id<CURR_LEVEL>(word_id, ctx_id)) {
                        //There is data found under this context
                        payload = m_M_gram_data[CURR_LEVEL - BASE::MGRAM_IDX_OFFSET][ctx_id];
                        return GPR_Enum::PAYLOAD_GPR;
                    } else {
                        //The context id could not be found
                        return GPR_Enum::FAILED_GPR;
                    }
                }

                /**
                 * Allows to retrieve the payload for the N gram defined by the end word_id and ctx_id.
                 * For more details @see LayeredTrieBase
                 */
                inline GPR_Enum get_n_gram_payload(const TShortId word_id, TLongId ctx_id,
                        T_M_Gram_Payload &payload) const {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctx_id, word_id);

                    //Search for the map for that context id
                    TNGramsMap::const_iterator result = pNGramMap->find(key);
                    if (result == pNGramMap->end()) {
                        //There is no data found under this context
                        return GPR_Enum::FAILED_GPR;
                    } else {
                        //There is data found under this context
                        payload.prob = result->second;
                        return GPR_Enum::PAYLOAD_GPR;
                    }
                }

                /**
                 * Allows to retrieve the probability and back-off weight of the unknown word
                 * @param payload the unknown word payload data
                 */
                inline void get_unk_word_payload(T_M_Gram_Payload & payload) const {
                    payload = m_1_gram_data[WordIndexType::UNKNOWN_WORD_ID];
                };

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
                TMGramAllocator * pMGramAlloc[BASE::NUM_M_GRAM_LEVELS];
                //The array of maps map storing M-grams for 1 < M < N
                TMGramsMap * pMGramMap[BASE::NUM_M_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                T_M_Gram_Payload * m_M_gram_data[BASE::NUM_M_GRAM_LEVELS];

                //The type of key,value pairs to be stored in the N Grams map
                typedef pair< const TLongId, TLogProbBackOff> TNGramEntry;
                //The typedef for the N Grams map allocator
                typedef GreedyMemoryAllocator< TNGramEntry > TNGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, TLogProbBackOff, std::hash<TLongId>, std::equal_to<TLongId>, TNGramAllocator > TNGramsMap;
                //The actual data storage for the N Grams
                TNGramAllocator * pNGramAlloc;
                //The map storing the N-Grams, they do not have back-off values
                TNGramsMap * pNGramMap;

                /**
                 * The copy constructor, is made private as we do not intend to copy this class objects
                 * @param orig the object to copy from
                 */
                C2DHybridTrie(const C2DHybridTrie & orig)
                : LayeredTrieBase<MAX_LEVEL, WordIndexType, __C2DHybridTrie::DO_BITMAP_HASH_CACHE>(orig.m_word_index), m_mgram_mem_factor(0.0), m_ngram_mem_factor(0.0), m_1_gram_data(NULL) {
                    throw Exception("ContextMultiHashMapTrie copy constructor must not be used, unless implemented!");
                };

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
        }
    }
}
#endif	/* CONTEXTMULTIHASHMAPTRIE_HPP */

