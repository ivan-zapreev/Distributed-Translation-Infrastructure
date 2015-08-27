/* 
 * File:   W2COrderedArrayTrie.hpp
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
 * Created on August 27, 2015, 08:33 AM
 */

#ifndef W2CORDEREDARRAYTRIE_HPP
#define	W2CORDEREDARRAYTRIE_HPP

#include <string>       // std::string
#include <cstdlib>      // std::calloc std::realloc std::free
#include <functional>   // std::function 
#include <cmath>        // std::log std::log10
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "ATrie.hpp"
#include "AWordIndex.hpp"
#include "ArrayUtils.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::array;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the Context to word array memory trie implementation class.
             * 
             * @param N the maximum number of levels in the trie.
             */
            template<TModelLevel N>
            class W2COrderedArrayTrie : public ATrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit W2COrderedArrayTrie(AWordIndex * const p_word_index);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * The basic destructor
                 */
                virtual ~W2COrderedArrayTrie();

            protected:

                /**
                 * This is a function type for the function that should compute the capacity increase
                 * @param the current capacity
                 * @return the capacity increase
                 */
                typedef std::function<float (const float) > TCapacityIncFunct;

                /**
                 * This structure is to store the word mapping to the data for
                 * the 1< M <= N grams. Demending on whether M == N or not this
                 * structure is to be instantiated with a different template
                 * parameter - defines the stored data.
                 * @param ptr the pointer to the storage array
                 * @param capacity the number of allocated elements
                 * @param size the number of used elements
                 * @param cio the context index offset for computing the next contex index.
                 */
                template<typename ARRAY_ELEM_TYPE>
                struct SSubArrayReference {
                    ARRAY_ELEM_TYPE * ptr;
                    TShortId capacity;
                    TShortId size;
                    TShortId cio;
                    typedef ARRAY_ELEM_TYPE TElemType;
                };

                /**
                 * This structure stores two things the context id
                 * and the corresponding probability/back-off data.
                 * It is used to store the M-gram data for levels 1 < M < N.
                 * @param ctxId the context id
                 * @param data the back-off and probability data
                 */
                typedef struct {
                    TShortId ctxId;
                    TProbBackOffEntry data;

                    operator TShortId() const {
                        return ctxId;
                    }
                } TCtxIdProbBackOffEntry;

                /**
                 * Stores the information about the context id and corresponding probability
                 * This data structure is to be used for the N-Gram data, as there are no back-offs
                 * It is used to store the N-gram data for the last Trie level N.
                 * @param ctxId the context id
                 * @param prob the probability data
                 */
                typedef struct {
                    TShortId ctxId;
                    TLogProbBackOff prob;

                    operator TShortId() const {
                        return ctxId;
                    }
                } TCtxIdProbEntry;

                //The entries for the M-grams to store information about the end words
                typedef SSubArrayReference<TCtxIdProbBackOffEntry> T_M_GramWordEntry;
                //The entries for the N-grams to store information about the end words
                typedef SSubArrayReference<TCtxIdProbEntry> T_N_GramWordEntry;

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntry & make_1_GramDataRef(const TShortId wordId) {
                    LOG_DEBUG2 << "Adding 1-gram with wordId: " << SSTR(wordId) << END_LOG;
                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TProbBackOffEntry & get_1_GramDataRef(const TShortId wordId) {
                    LOG_DEBUG2 << "Getting 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntry& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    LOG_DEBUG2 << "Adding\t" << SSTR(level) << "-gram with ctxId:\t"
                            << SSTR(ctxId) << ", wordId:\t" << SSTR(wordId) << END_LOG;

                    //Get the sub-array reference. 
                    TCtxIdProbBackOffEntry & ref = make_M_N_GramEntry<T_M_GramWordEntry>(m_M_gram_word_2_data[level - MGRAM_IDX_OFFSET], wordId);

                    //Store the context and word ids
                    ref.ctxId = ctxId;

                    //Return the reference to the newly allocated element
                    return ref.data;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TProbBackOffEntry& get_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the entry
                    const TCtxIdProbBackOffEntry & ref = get_M_N_GramEntry<T_M_GramWordEntry>(N, m_M_gram_word_2_data[level - MGRAM_IDX_OFFSET], wordId, ctxId);

                    //Return the reference to the probability and back-off structure
                    return ref.data;
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    LOG_DEBUG2 << "Adding\t" << SSTR(N) << "-gram with ctxId:\t"
                            << SSTR(ctxId) << ", wordId:\t" << SSTR(wordId) << END_LOG;

                    //Get the sub-array reference. 
                    TCtxIdProbEntry & ref = make_M_N_GramEntry<T_N_GramWordEntry>(m_N_gram_word_2_data, wordId);

                    //Store the context and word ids
                    ref.ctxId = ctxId;

                    //return the reference to the probability
                    return ref.prob;
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TLogProbBackOff& get_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    LOG_DEBUG2 << "Getting " << SSTR(N) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the entry
                    const TCtxIdProbEntry & ref = get_M_N_GramEntry<T_N_GramWordEntry>(N, m_N_gram_word_2_data, wordId, ctxId);

                    //Return the reference to the probability
                    return ref.prob;
                };

                virtual bool isPost_Grams(const TModelLevel level) {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per word by context id, see
                    //post_M_Grams, and post_N_Grams methods below.
                    return (level > MIN_NGRAM_LEVEL) || ATrie<N>::isPost_Grams(level);
                }

                /**
                 * The purpose of this local function is three fold:
                 * 1. First we compute the context index offset values.
                 * 2. Second we re-order the context data arrays per word.
                 * 3. Free the unneeded memory allocated earlier.
                 * @param WORD_ENTRY_TYPE word array element type
                 * @param wordsArray the word array to work with
                 */
                template<typename WORD_ENTRY_TYPE>
                void post_M_N_Grams(WORD_ENTRY_TYPE * wordsArray) {
                    //Define and initialize the current context index offset.
                    //The initial value is 1, although in this Trie it should
                    //not matter much, but it is better to reserve 0 for
                    //an undefined context value
                    TShortId cio = FIRST_VALID_CTX_ID;

                    //Iterate through all the wordId sub-array mappings in the level and sort sub arrays
                    for (TShortId wordId = MIN_KNOWN_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        //First get the sub-array reference. 
                        WORD_ENTRY_TYPE & ref = wordsArray[wordId];

                        //Assign the context index offset
                        ref.cio = cio;
                        //Compute the next context index offset, for the next word
                        cio += ref.size;

                        //Check that the data for the given word is available
                        if ((ref.ptr != NULL) && (ref.size > 0)) {
                            //Deallocate the unneeded memory, the false flag indicates that we need reduction.
                            reallocateWordData<WORD_ENTRY_TYPE, false>(ref);

                            //Order the N-gram array as it is unordered and we will binary search it later!
                            //Note: We do not use qsort as it has worse performance than this method.
                            sort<typename WORD_ENTRY_TYPE::TElemType, TShortId > (ref.ptr, ref.ptr + ref.size);
                        }
                    }
                }

                virtual void post_M_Grams(const TModelLevel level) {
                    //Call the base class method first
                    ATrie<N>::post_N_Grams();

                    //Sort the level's data
                    post_M_N_Grams<T_M_GramWordEntry>(m_M_gram_word_2_data[level - MGRAM_IDX_OFFSET]);
                }

                virtual void post_N_Grams() {
                    //Call the base class method first
                    ATrie<N>::post_N_Grams();

                    //Sort the level's data
                    post_M_N_Grams<T_N_GramWordEntry>(m_N_gram_word_2_data);
                };

            private:
                //The offset, relative to the M-gram level M for the mgram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = N - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = N - 1;

                //Compute the N-gram index in m_MN_gram_size and  m_MN_gram_idx_cnts
                static const TModelLevel N_GRAM_IDX = N - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;

                //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                static const TShortId EXTRA_NUMBER_OF_WORD_IDs = 2;

                //Stores the number of used word ids
                TShortId m_num_word_ids;

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a two dimensional array
                T_M_GramWordEntry * m_M_gram_word_2_data[NUM_M_GRAM_LEVELS];

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a one dimensional array
                T_N_GramWordEntry * m_N_gram_word_2_data;

                //Stores the pointer to the capacity computing function
                TCapacityIncFunct m_get_capacity_inc_func;

                /**
                 * For a M-gram allows to create a new context entry for the given word id.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the array where the word entries of this level are stored
                 * @param wordId the word id we need the new context entry from
                 */
                template<typename WORD_ENTRY_TYPE>
                typename WORD_ENTRY_TYPE::TElemType & make_M_N_GramEntry(WORD_ENTRY_TYPE* wordsArray, const TShortId & wordId) {
                    LOG_DEBUG2 << "Making entry for \tM-gram with wordId:\t" << SSTR(wordId) << END_LOG;

                    //First get the sub-array reference. 
                    WORD_ENTRY_TYPE & ref = wordsArray[wordId];

                    //If sanity checks are enabled then check for a null pointer!
                    if (DO_SANITY_CHECKS && (ref.ptr == NULL)) {
                        stringstream msg;
                        msg << "The M-gram wordId: " << SSTR(wordId)
                                << " data array is NULL, capacity: " << ref.capacity << " !";
                        throw Exception(msg.str());
                    }

                    LOG_DEBUG2 << "Got the wordId: " << SSTR(wordId) << " data entry, capacity = "
                            << SSTR(ref.capacity) << ", size = " << SSTR(ref.size) << END_LOG;

                    //Check if we need to increase the capacity!
                    if (ref.size == ref.capacity) {
                        reallocateWordData<WORD_ENTRY_TYPE>(ref);
                    }

                    //Return the next new element and increase the size!
                    return ref.ptr[ref.size++];
                };

                /**
                 * For the given M-gram defined by the word id and a context id it allows to retrieve local index where the m-gram's entry is stored.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordEntry the word entry to search in
                 * @param ctxId the context id we are after
                 * @throw out_of_range if the index could not be found
                 */
                template<typename WORD_ENTRY_TYPE>
                const TShortId get_M_N_GramLocalEntryIdx(const WORD_ENTRY_TYPE & ref, const TShortId & ctxId) {
                    LOG_DEBUG2 << "Getting sub arr data for ctxId: " << SSTR(ctxId) << END_LOG;

                    //Check if there is data to search in
                    if ((ref.ptr != NULL) && (ref.size > 0)) {
                        TShortId localIdx = UNDEFINED_ARR_IDX;
                        //The data is available search for the word index in the array
                        if (binarySearch<typename WORD_ENTRY_TYPE::TElemType, TShortId, TShortId>(ref.ptr, 0, ref.size - 1, ctxId, localIdx)) {
                            return localIdx;
                        } else {
                            LOG_DEBUG1 << "Unable to find M-gram context id for a word, prev ctxId: "
                                    << SSTR(ctxId) << ", ctxId range: [" << SSTR(ref.ptr[0].ctxId)
                                    << ", " << SSTR(ref.ptr[ref.size - 1].ctxId) << "]" << END_LOG;
                            throw out_of_range("not found");
                        }
                    } else {
                        LOG_DEBUG1 << "Unable to find M-gram word id data for a word, nothing is present!" << END_LOG;
                        throw out_of_range("not found");
                    }
                }

                /**
                 * For the given M-gram defined by the word id and a context id it allows to retrieve the data entry.
                 * This method words for 1 < M <= N. The level parameter is only used for debugging.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param level the Trie level
                 * @param wordsArray the array where the word entries of this level are stored
                 * @param wordId the word id we need the to find the context entry by
                 * @param ctxId the context id we are after
                 */
                template<typename WORD_ENTRY_TYPE>
                const typename WORD_ENTRY_TYPE::TElemType & get_M_N_GramEntry(const TModelLevel & level, const WORD_ENTRY_TYPE* wordsArray, const TShortId & wordId, const TShortId & ctxId) {
                    LOG_DEBUG2 << "Getting sub arr data for " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << END_LOG;
                    //Get the sub-array reference. 
                    const WORD_ENTRY_TYPE & ref = wordsArray[wordId];

                    //Get the local entry index
                    const TShortId localIdx = get_M_N_GramLocalEntryIdx<WORD_ENTRY_TYPE>(ref, ctxId);

                    //Return the data located by the local index
                    return ref.ptr[localIdx];
                }

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M <= N!
                 * 
                 * @param wordId the current word id
                 * @param ctxId the previous context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw out_of_range in case the context can not be computed, e.g. does not exist.
                 */
                inline TLongId getContextId(TShortId wordId, TLongId ctxId, const TModelLevel level) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Searching for the context id of " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << ", ctxId: "
                            << SSTR(ctxId) << END_LOG;

                    //First get the sub-array reference. 
                    const T_M_GramWordEntry & ref = m_M_gram_word_2_data[mgram_idx][wordId];

                    //Get the local entry index
                    const TShortId localIdx = get_M_N_GramLocalEntryIdx(ref, ctxId);

                    LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                            << ", with ptr: " << SSTR(ref.ptr) << ", size: "
                            << SSTR(ref.size) << END_LOG;

                    //return the context id which is the sum of the
                    //local index and the context index offset
                    return ref.cio + localIdx;
                }

                /**
                 * Allows to allocate word related data per word for the given M/N gram level
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the reference to the word entry array to initialize
                 * @param numMGrams the number of M-grams on this level
                 * @param numWords the number of words in the Trie
                 */
                template<typename WORD_ENTRY_TYPE>
                void preAllocateWordsData(WORD_ENTRY_TYPE* & wordsArray, const TShortId numMGrams, const TShortId numWords) {
                    //Allocate memory and clean it for the liven level = (i + MGRAM_IDX_OFFSET)
                    wordsArray = new WORD_ENTRY_TYPE[m_num_word_ids];
                    memset(wordsArray, 0, m_num_word_ids * sizeof (WORD_ENTRY_TYPE));

                    //Compute the average number of data elements per word on the given M-gram level
                    const float avg_num_elems = ((float) numMGrams) / ((float) numWords);
                    /*
                    //Compute the corrected number of elements to preallocate, minimum __W2COrderedArrayTrie::MIN_MEM_INC_NUM.
                    const size_t fact_num_elems = max(static_cast<size_t> (avg_num_elems * __W2COrderedArrayTrie::INIT_MEM_ALLOC_PRCT), __W2COrderedArrayTrie::MIN_MEM_INC_NUM);

                    //Allocate data per word, use the calloc in order to be able to realloc it!
                    for (TShortId wordId = MIN_KNOWN_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        wordsArray[wordId].ptr = (typename WORD_ENTRY_TYPE::TElemType*) calloc(fact_num_elems, sizeof (typename WORD_ENTRY_TYPE::TElemType));
                        wordsArray[wordId].capacity = fact_num_elems;

                        //Do the null pointer check if sanity
                        if (DO_SANITY_CHECKS && (fact_num_elems > 0) && (wordsArray[wordId].ptr == NULL)) {
                            stringstream msg;
                            msg << "Ran out of memory when trying to allocate "
                                    << fact_num_elems << " data elements for wordId: " << wordId;
                            throw Exception(msg.str());
                        }
                    }*/
                }

                /**
                 * Compute the new capacity given the provided one, this function used
                 * the capacity increase function stored in m_get_capacity_inc_func.
                 * @param capacity the current capacity
                 * @return the proposed capacity increase, will be at least __W2COrderedArrayTrie::MIN_MEM_INC_NUM
                 */
                inline const size_t computeNewCapacity(const size_t capacity) {
                    //Get the float capacity value, make it minimum of one element to avoid problems
                    const float fcap = (capacity > 0) ? (float) capacity : 1.0;
                    const size_t cap_inc = (size_t) m_get_capacity_inc_func(fcap);
                    return capacity + max(cap_inc, __W2COrderedArrayTrie::MIN_MEM_INC_NUM);
                }

                /**
                 * Allows to reallocate the word entry data increasing or decreasing its capacity.
                 * @param WORD_ENTRY_TYPE the entry type
                 * @param isIncrease if true then the memory will be attempted to increase, otherwise decrease
                 * @param wordEntry the entry to work with
                 */
                template<typename WORD_ENTRY_TYPE, bool isIncrease = true >
                void reallocateWordData(WORD_ENTRY_TYPE & wordEntry) {
                    size_t new_capacity;

                    LOG_DEBUG3 << "Memory reallocation request: "
                            << ((isIncrease) ? "increase" : "decrease") << END_LOG;

                    //Compute the new number of elements
                    if (isIncrease) {
                        //Compute the new capacity
                        new_capacity = computeNewCapacity(wordEntry.capacity);
                    } else {
                        //Decrease the capacity to the current size, remove the unneeded
                        new_capacity = wordEntry.size;
                    }

                    LOG_DEBUG2 << "The estimated new capacity is " << SSTR(new_capacity)
                            << ", the old capacity was " << SSTR(wordEntry.capacity)
                            << ", used size: " << SSTR(wordEntry.size) << END_LOG;

                    //Reallocate memory, potentially we get a new pointer!
                    wordEntry.ptr = (typename WORD_ENTRY_TYPE::TElemType*) realloc(wordEntry.ptr, new_capacity * sizeof (typename WORD_ENTRY_TYPE::TElemType));

                    //Clean the newly allocated memory
                    if (isIncrease) {
                        const size_t new_num_elem = (new_capacity - wordEntry.capacity);
                        memset(wordEntry.ptr + wordEntry.size, 0,
                                new_num_elem * sizeof (typename WORD_ENTRY_TYPE::TElemType));
                    }

                    //Set the new capacity in
                    wordEntry.capacity = new_capacity;

                    //Do the null pointer check if sanity
                    if (DO_SANITY_CHECKS && isIncrease && (new_capacity > 0) && (wordEntry.ptr == NULL)) {

                        stringstream msg;
                        msg << "Ran out of memory when trying to allocate "
                                << new_capacity << " data elements for a wordId";
                        throw Exception(msg.str());
                    }
                }

                /**
                 * Allows to allocate word related data per word for the given M/N gram level
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordEntry the reference to the word entry array to initialize
                 * @param numMGrams the number of M-grams on this level
                 * @param numWords the number of words in the Trie
                 */
                template<typename WORD_ENTRY_TYPE>
                void deAllocateWordsData(WORD_ENTRY_TYPE* & wordEntry) {
                    //Allocate data per word, use the calloc in order to be able to realloc it!
                    for (TShortId wordId = MIN_KNOWN_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        if (wordEntry[wordId].ptr != NULL) {
                            free(wordEntry[wordId].ptr);
                        }
                    }
                    delete[] wordEntry;
                }
            };

            typedef W2COrderedArrayTrie<MAX_NGRAM_LEVEL> TW2COrderedArrayTrie_N5;
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

