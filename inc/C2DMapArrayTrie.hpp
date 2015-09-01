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
#include "ATrie.hpp"
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
             * speeding up queries, as they allow us to realize (wordId, ctxId) to ctxId
             * in the most efficient manner. The lookup should be just O(1) whereas in
             * the lookup is O(log(n)), as we need to use binary searches there.
             */
            template<TModelLevel N>
            class C2DMapArrayTrie : public ATrie<N> {
            public:
                //Stores the offset for the MGram index, this is the number of M-gram levels stored elsewhere
                static const TModelLevel MGRAM_IDX_OFFSET = 2;

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
                explicit C2DMapArrayTrie(AWordIndex * const _pWordIndex,
                        const float _mGramMemFactor = __C2DMapArrayTrie::UM_M_GRAM_MEMORY_FACTOR,
                        const float _nGramMemFactor = __C2DMapArrayTrie::UM_N_GRAM_MEMORY_FACTOR);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * The basic destructor
                 */
                virtual ~C2DMapArrayTrie();

            protected:

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntry & make_1_GramDataRef(const TShortId wordId) {
                    //Get the word probability and back-off data reference
                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual bool get_1_GramDataRef(const TShortId wordId, const TProbBackOffEntry ** ppData) {
                    //The data is always present.
                    *ppData = &m_1_gram_data[wordId];
                    return true;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntry & make_M_GramDataRef(const TModelLevel level, const TShortId wordId, TLongId ctxId) {
                    //Store the N-tires from length 2 on and indexing starts
                    //with 0, therefore "level-2". Get/Create the mapping for this
                    //word in the Trie level of the N-gram

                    //Obtain the context key and then create a new mapping
                    const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);
                    //Get the next context id
                    const TModelLevel idx = level - ATrie<N>::MGRAM_IDX_OFFSET;
                    TShortId nextCtxId = m_M_gram_next_ctx_id[idx]++;
                    //Store the context mapping inside the map
                    pMGramMap[idx]->operator[](key) = nextCtxId;

                    //Return the reference to the piece of memory
                    return m_M_gram_data[level - ATrie<N>::MGRAM_IDX_OFFSET][nextCtxId];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual bool get_M_GramDataRef(const TModelLevel level, const TShortId wordId,
                        TLongId ctxId, const TProbBackOffEntry **ppData) {
                    //Get the next context id
                    if (getContextId(wordId, ctxId, level)) {
                        //There is data found under this context
                        *ppData = &m_M_gram_data[level - ATrie<N>::MGRAM_IDX_OFFSET][ctxId];
                        return true;
                    } else {
                        //The context id could not be found
                        return false;
                    }
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff & make_N_GramDataRef(const TShortId wordId, TLongId ctxId) {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);
                    return pNGramMap->operator[](key);
                };

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * For more details @see ATrie
                 */
                virtual bool get_N_GramProb(const TShortId wordId, TLongId ctxId,
                        TLogProbBackOff & prob) {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);

                    //Search for the map for that context id
                    TNGramsMap::const_iterator result = pNGramMap->find(key);
                    if (result == pNGramMap->end()) {
                        //There is no data found under this context
                        return false;
                    } else {
                        //There is data found under this context
                        prob = result->second;
                        return true;
                    }
                };

            private:

                //The M-Gram memory factor needed for the greedy allocator for the unordered_map
                const float mGramMemFactor;
                //The N-Gram memory factor needed for the greedy allocator for the unordered_map
                const float nGramMemFactor;
                
                //Stores the context id counters per M-gram level: 1 < M < N
                TShortId m_M_gram_next_ctx_id[ATrie<N>::NUM_M_GRAM_LEVELS];
                //Stores the context id counters per M-gram level: 1 < M <= N
                TShortId m_M_gram_num_ctx_ids[ATrie<N>::NUM_M_N_GRAM_LEVELS];

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //The type of key,value pairs to be stored in the M Grams map
                typedef pair< const TLongId, TShortId> TMGramEntry;
                //The typedef for the M Grams map allocator
                typedef GreedyMemoryAllocator< TMGramEntry > TMGramAllocator;
                //The N Grams map type
                typedef unordered_map<TLongId, TShortId, std::hash<TLongId>, std::equal_to<TLongId>, TMGramAllocator > TMGramsMap;
                //The actual data storage for the M Grams for 1 < M < N
                TMGramAllocator * pMGramAlloc[ATrie<N>::NUM_M_GRAM_LEVELS];
                //The array of maps map storing M-grams for 1 < M < N
                TMGramsMap * pMGramMap[ATrie<N>::NUM_M_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                TProbBackOffEntry * m_M_gram_data[ATrie<N>::NUM_M_GRAM_LEVELS];

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
                C2DMapArrayTrie(const C2DMapArrayTrie & orig)
                : ATrie<N>(NULL, NULL), mGramMemFactor(0.0), nGramMemFactor(0.0), m_1_gram_data(NULL) {
                    throw Exception("ContextMultiHashMapTrie copy constructor must not be used, unless implemented!");
                };

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateOGrams(const size_t counts[N]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateMGrams(const size_t counts[N]);

                /**
                 * This method must used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @param counts the counts for the number of elements of each gram level
                 */
                void preAllocateNGrams(const size_t counts[N]);

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M < N!
                 * 
                 * @param wordId the current word id
                 * @param ctxId [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M, default UNDEF_NGRAM_LEVEL
                 * @return the resulting context
                 * @throw nothing
                 */
                inline bool getContextId(const TShortId wordId, TLongId & ctxId, const TModelLevel level) {
                    const TLongId key = TShortId_TShortId_2_TLongId(ctxId, wordId);

                    //Search for the map for that context id
                    const TModelLevel idx = level - ATrie<N>::MGRAM_IDX_OFFSET;
                    TMGramsMap::const_iterator result = pMGramMap[idx]->find(key);
                    if (result == pMGramMap[idx]->end()) {
                        //There is no data found under this context
                        return false;
                    } else {
                        //Update the context with the found value uf the next context
                        ctxId = result->second;
                        //The context can always be computed
                        return true;
                    }
                }

            };

            typedef C2DMapArrayTrie<MAX_NGRAM_LEVEL> TC2DMapArrayTrie_N5;
        }
    }
}
#endif	/* CONTEXTMULTIHASHMAPTRIE_HPP */

