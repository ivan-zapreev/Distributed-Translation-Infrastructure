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
#include "ALayeredTrie.hpp"
#include "AWordIndex.hpp"
#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::array;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::__W2COrderedArrayTrie;

namespace uva {
    namespace smt {
        namespace tries {
            namespace __W2COrderedArrayTrie {

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
                } TCtxIdProbBackOffEntry;

                /**
                 * This is the less operator implementation
                 * @param one the first object to compare
                 * @param two the second object to compare
                 * @return true if ctxId of one is smaller than ctxId of two, otherwise false
                 */
                inline bool operator<(const TCtxIdProbBackOffEntry& one, const TCtxIdProbBackOffEntry& two) {
                    return one.ctxId < two.ctxId;
                }

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
                } TCtxIdProbEntry;

                /**
                 * This is the less operator implementation
                 * @param one the first object to compare
                 * @param two the second object to compare
                 * @return true if ctxId of one is smaller than ctxId of two, otherwise false
                 */
                inline bool operator<(const TCtxIdProbEntry& one, const TCtxIdProbEntry& two) {
                    return one.ctxId < two.ctxId;
                }
            }

            /**
             * This is the Context to word array memory trie implementation class.
             * 
             * @param N the maximum number of levels in the trie.
             */
            template<TModelLevel N>
            class W2COrderedArrayTrie : public ALayeredTrie<N> {
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
                 * This class is to store the word mapping to the data for
                 * the 1< M <= N grams. Demending on whether M == N or not this
                 * structure is to be instantiated with a different template
                 * parameter - defines the stored data.
                 * @param ptr the pointer to the storage array
                 * @param capacity the number of allocated elements
                 * @param size the number of used elements
                 * @param cio the context index offset for computing the next contex index.
                 */
                template<typename ARRAY_ELEM_TYPE>
                class WordDataEntry : public DynamicStackArray<ARRAY_ELEM_TYPE> {
                public:
                    TShortId cio;
                    typedef ARRAY_ELEM_TYPE TElemType;
                };

                //The entries for the M-grams to store information about the end words

                typedef WordDataEntry<TCtxIdProbBackOffEntry> T_M_GramWordEntry;
                //The entries for the N-grams to store information about the end words
                typedef WordDataEntry<TCtxIdProbEntry> T_N_GramWordEntry;

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
                virtual bool get_1_GramDataRef(const TShortId wordId, const TProbBackOffEntry ** ppData) {
                    LOG_DEBUG2 << "Getting 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                    *ppData = &m_1_gram_data[wordId];

                    //The data should always be present, unless of course this is a bad index!

                    return true;
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
                    typename T_M_GramWordEntry::TElemType & ref = make_M_N_GramEntry<T_M_GramWordEntry>(m_M_gram_word_2_data[level - ALayeredTrie<N>::MGRAM_IDX_OFFSET], wordId);

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
                virtual bool get_M_GramDataRef(const TModelLevel level, const TShortId wordId,
                        const TLongId ctxId, const TProbBackOffEntry **ppData) {
                    LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the entry
                    const typename T_M_GramWordEntry::TElemType * pEntry;
                    if (get_M_N_GramEntry<T_M_GramWordEntry>(level, m_M_gram_word_2_data[level - ALayeredTrie<N>::MGRAM_IDX_OFFSET], wordId, ctxId, &pEntry)) {
                        //Return the pointer to the probability and back-off structure
                        *ppData = &pEntry->data;
                        return true;
                    } else {
                        //The data could not be found

                        return false;
                    }
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    LOG_DEBUG2 << "Adding " << SSTR(N) << "-gram with ctxId: "
                            << SSTR(ctxId) << ", wordId: " << SSTR(wordId) << END_LOG;

                    //Get the sub-array reference. 
                    typename T_N_GramWordEntry::TElemType & ref = make_M_N_GramEntry<T_N_GramWordEntry>(m_N_gram_word_2_data, wordId);

                    //Store the context and word ids
                    ref.ctxId = ctxId;

                    //return the reference to the probability

                    return ref.prob;
                };

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * For more details @see ATrie
                 */
                virtual bool get_N_GramProb(const TShortId wordId, const TLongId ctxId,
                        TLogProbBackOff & prob) {
                    LOG_DEBUG2 << "Getting " << SSTR(N) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the entry
                    const typename T_N_GramWordEntry::TElemType * pEntry;
                    if (get_M_N_GramEntry<T_N_GramWordEntry>(N, m_N_gram_word_2_data, wordId, ctxId, &pEntry)) {
                        //Return the reference to the probability
                        prob = pEntry->prob;
                        return true;
                    } else {

                        return false;
                    }
                };

                virtual bool isPost_Grams(const TModelLevel level) {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per word by context id, see
                    //post_M_Grams, and post_N_Grams methods below.

                    return (level > ONE_GRAM_LEVEL) || ALayeredTrie<N>::isPost_Grams(level);
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
                    TShortId cio = ALayeredTrie<N>::FIRST_VALID_CTX_ID;

                    //Iterate through all the wordId sub-array mappings in the level and sort sub arrays
                    for (TShortId wordId = AWordIndex::UNDEFINED_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        //First get the sub-array reference. 
                        WORD_ENTRY_TYPE & ref = wordsArray[wordId];

                        //Assign the context index offset
                        ref.cio = cio;
                        //Compute the next context index offset, for the next word
                        cio += ref.get_size();

                        //Reduce capacity if there is unused memory
                        ref.shrink();

                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort();
                    }
                }

                virtual void post_M_Grams(const TModelLevel level) {
                    //Call the base class method first

                    ALayeredTrie<N>::post_N_Grams();

                    //Sort the level's data
                    post_M_N_Grams<T_M_GramWordEntry>(m_M_gram_word_2_data[level - ALayeredTrie<N>::MGRAM_IDX_OFFSET]);
                }

                virtual void post_N_Grams() {
                    //Call the base class method first

                    ALayeredTrie<N>::post_N_Grams();

                    //Sort the level's data
                    post_M_N_Grams<T_N_GramWordEntry>(m_N_gram_word_2_data);
                };

            private:

                //Stores the number of used word ids
                TShortId m_num_word_ids;

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a two dimensional array
                T_M_GramWordEntry * m_M_gram_word_2_data[ALayeredTrie<N>::NUM_M_GRAM_LEVELS];

                //Stores the M-gram word to data mappings for: 1 < M < N
                //This is a one dimensional array
                T_N_GramWordEntry * m_N_gram_word_2_data;

                //Stores the memory increase strategy object
                MemIncreaseStrategy * m_p_mem_strat;

                /**
                 * For a M-gram allows to create a new context entry for the given word id.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the array where the word entries of this level are stored
                 * @param wordId the word id we need the new context entry from
                 */
                template<typename WORD_ENTRY_TYPE>
                typename WORD_ENTRY_TYPE::TElemType & make_M_N_GramEntry(WORD_ENTRY_TYPE* wordsArray, const TShortId & wordId) {
                    LOG_DEBUG2 << "Making entry for M-gram with wordId:\t" << SSTR(wordId) << END_LOG;

                    //Return the next new element new/free!
                    return wordsArray[wordId].get_new();
                };

                /**
                 * For the given M-gram defined by the word id and a context id it allows to retrieve local index where the m-gram's entry is stored.
                 * This method words for 1 < M <= N.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordEntry the word entry to search in
                 * @param ctxId the context id we are after
                 * @param localIdx the [out] parameter which is the found local array index
                 * @return true if the index was found otherwise false
                 * @throw nothing
                 */
                template<typename WORD_ENTRY_TYPE>
                bool get_M_N_GramLocalEntryIdx(const WORD_ENTRY_TYPE & ref, const TShortId ctxId, TShortId & localIdx) {
                    LOG_DEBUG2 << "Searching word data entry for ctxId: " << SSTR(ctxId) << END_LOG;

                    //Check if there is data to search in
                    if (ref.has_data()) {
                        //The data is available search for the word index in the array
                        if (bsearch_ctxId<typename WORD_ENTRY_TYPE::TElemType > (ref.get_data(), 0, ref.get_size() - 1, ctxId, localIdx)) {
                            LOG_DEBUG2 << "Found sub array local index = " << SSTR(localIdx) << END_LOG;
                            return true;
                        } else {
                            LOG_DEBUG1 << "Unable to find M-gram context id for a word, prev ctxId: "
                                    << SSTR(ctxId) << ", ctxId range: [" << SSTR(ref[0].ctxId)
                                    << ", " << SSTR(ref[ref.get_size() - 1].ctxId) << "]" << END_LOG;
                            return false;
                        }
                    } else {
                        LOG_DEBUG1 << "Unable to find M-gram word id data for a word, nothing is present!" << END_LOG;

                        return false;
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
                 * @return true if the data was found, otherwise false
                 * @throw nothing
                 */
                template<typename WORD_ENTRY_TYPE>
                bool get_M_N_GramEntry(const TModelLevel & level, const WORD_ENTRY_TYPE* wordsArray,
                        const TShortId & wordId, const TShortId & ctxId, const typename WORD_ENTRY_TYPE::TElemType **ppData) {
                    LOG_DEBUG2 << "Getting sub arr data for " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << END_LOG;
                    //Get the sub-array reference. 
                    const WORD_ENTRY_TYPE & ref = wordsArray[wordId];

                    //Check that if this is the 2-Gram case and the previous context
                    //id is 0 then it is the unknown word id, at least this is how it
                    //is now in ATrie implementation, so we need to do a warning!
                    if (DO_SANITY_CHECKS && (level == TWO_GRAM_LEVEL) && (ctxId < AWordIndex::MIN_KNOWN_WORD_ID)) {
                        LOG_WARNING << "Perhaps we are being paranoid but there "
                                << "seems to be a problem! The " << SSTR(level) << "-gram ctxId: "
                                << SSTR(ctxId) << " is equal to an undefined(" << SSTR(AWordIndex::UNDEFINED_WORD_ID)
                                << ") or unknown(" << SSTR(AWordIndex::UNKNOWN_WORD_ID) << ") word ids!" << END_LOG;
                    }

                    //Get the local entry index
                    TShortId localIdx;
                    if (get_M_N_GramLocalEntryIdx<WORD_ENTRY_TYPE>(ref, ctxId, localIdx)) {
                        //Return the pointer to the data located by the local index
                        *ppData = &ref[localIdx];
                        return true;
                    } else {
                        LOG_DEBUG2 << "Unable to find data entry for " << SSTR(level) << "-gram ctxId: "
                                << SSTR(ctxId) << ", wordId: " << SSTR(wordId) << END_LOG;

                        return false;
                    }
                }

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M < N!
                 * 
                 * @param wordId the current word id
                 * @param ctxId [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw nothing.
                 */
                inline bool getContextId(const TShortId wordId, TLongId & ctxId, const TModelLevel level) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - ALayeredTrie<N>::MGRAM_IDX_OFFSET;

                    if (DO_SANITY_CHECKS && ((level == N) || (mgram_idx < 0))) {
                        stringstream msg;
                        msg << "Unsupported level id: " << level;
                        throw Exception(msg.str());
                    }

                    LOG_DEBUG2 << "Searching next ctxId for " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << ", ctxId: "
                            << SSTR(ctxId) << END_LOG;

                    //First get the sub-array reference. 
                    const T_M_GramWordEntry & ref = m_M_gram_word_2_data[mgram_idx][wordId];

                    if (DO_SANITY_CHECKS && ref.has_data()) {
                        LOG_DEBUG3 << "ref.size: " << SSTR(ref.get_size()) << ", ref.cio: "
                                << SSTR(ref.cio) << ", ctxId range: [" << SSTR(ref[0].ctxId) << ", "
                                << SSTR(ref[ref.get_size() - 1].ctxId) << "]" << END_LOG;
                    }

                    //Check that if this is the 2-Gram case and the previous context
                    //id is 0 then it is the unknown word id, at least this is how it
                    //is now in ATrie implementation, so we need to do a warning!
                    if (DO_SANITY_CHECKS && (level == TWO_GRAM_LEVEL) && (ctxId < AWordIndex::MIN_KNOWN_WORD_ID)) {
                        LOG_WARNING << "Perhaps we are being paranoid but there "
                                << "seems to be a problem! The " << SSTR(level) << "-gram ctxId: "
                                << SSTR(ctxId) << " is equal to an undefined(" << SSTR(AWordIndex::UNDEFINED_WORD_ID)
                                << ") or unknown(" << SSTR(AWordIndex::UNKNOWN_WORD_ID) << ") word ids!" << END_LOG;
                    }

                    //Get the local entry index and then use it to compute the next context id
                    TShortId localIdx;
                    //If the local entry index could be found then compute the next ctxId
                    if (get_M_N_GramLocalEntryIdx(ref, ctxId, localIdx)) {
                        LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                                << ", size = " << SSTR(ref.get_size()) << ", localIdx = "
                                << SSTR(localIdx) << ", resulting ctxId = "
                                << SSTR(ref.cio + localIdx) << END_LOG;

                        //The next ctxId is the sum of the local index and the context index offset
                        ctxId = ref.cio + localIdx;
                        return true;
                    } else {
                        //The local index could not be found
                        return false;
                    }
                }

                /**
                 * Allows to allocate word related data per word for the given M/N gram level
                 * Depends on the global __W2COrderedArrayTrie::PRE_ALLOCATE_MEMORY if true
                 * then will preallocate some memory for each word bucket! Default is false.
                 * Depending on the number of words and n-grams in the given Trie level,
                 * plus the value of:
                 *      __W2COrderedArrayTrie::INIT_MEM_ALLOC_PRCT
                 * This can have a drastic influence on MAX RSS.
                 * @param WORD_ENTRY_TYPE the word entry type
                 * @param wordsArray the reference to the word entry array to initialize
                 * @param numMGrams the number of M-grams on this level
                 * @param numWords the number of words in the Trie
                 */
                template<typename WORD_ENTRY_TYPE>
                void preAllocateWordsData(WORD_ENTRY_TYPE* & wordsArray, const TShortId numMGrams, const TShortId numWords) {
                    //Allocate dynamic array entries and initialize it with the memory increase strategy
                    wordsArray = new WORD_ENTRY_TYPE[m_num_word_ids];

                    //Compute the initial capacity
                    size_t capacity = 0;
                    if (__W2COrderedArrayTrie::PRE_ALLOCATE_MEMORY) {
                        //Compute the average number of data elements per word on the given M-gram level
                        const float avg_num_elems = ((float) numMGrams) / ((float) numWords);
                        //Compute the corrected number of elements to preallocate, minimum __W2COrderedArrayTrie::MIN_MEM_INC_NUM.
                        capacity = max(static_cast<size_t> (avg_num_elems * __W2COrderedArrayTrie::INIT_MEM_ALLOC_PRCT),
                                __W2COrderedArrayTrie::MIN_MEM_INC_NUM);
                    }

                    //Set the memory strategy and capacity into the word entries
                    for (TShortId wordId = AWordIndex::MIN_KNOWN_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        wordsArray[wordId].set_mem_strat(m_p_mem_strat, capacity);
                    }
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

                    LOG_DEBUG2 << "Memory reallocation request: "
                            << ((isIncrease) ? "increase" : "decrease") << END_LOG;

                    //Compute the new number of elements
                    if (isIncrease) {
                        //Compute the new capacity
                        new_capacity = m_p_mem_strat->computeNewCapacity(wordEntry.capacity);
                        LOG_DEBUG2 << "Computed new capacity is: " << new_capacity << END_LOG;
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
                        memset(wordEntry.ptr + wordEntry.capacity, 0,
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
                    for (TShortId wordId = AWordIndex::MIN_KNOWN_WORD_ID; wordId < m_num_word_ids; wordId++) {
                        if (wordEntry[wordId].ptr != NULL) {
                            free(wordEntry[wordId].ptr);
                        }
                    }
                    delete[] wordEntry;
                }
            };
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

