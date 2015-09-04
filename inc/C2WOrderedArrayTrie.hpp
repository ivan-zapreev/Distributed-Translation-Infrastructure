/* 
 * File:   C2WOrderedArrayTrie.hpp
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
 * Created on August 25, 2015, 11:10 AM
 */

#ifndef C2WORDEREDARRAYTRIE_HPP
#define	C2WORDEREDARRAYTRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "ALayeredTrie.hpp"
#include "AWordIndex.hpp"
#include "ArrayUtils.hpp"

using namespace std;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::utils::array;

namespace uva {
    namespace smt {
        namespace tries {
            namespace __C2WOrderedArrayTrie {

                /**
                 * Stores the information about the context id, word id and corresponding probability
                 * This data structure is to be used for the N-Gram data, as there are no back-offs
                 * It is used to store the N-gram data for the last Trie level N.
                 * @param ctxId the context id
                 * @param wordId the word id
                 * @param prob the probability data
                 */
                typedef struct {
                    TShortId wordId;
                    TShortId ctxId;
                    TLogProbBackOff prob;
                } TCtxIdProbData;

                /**
                 * This is the less operator implementation
                 * @param one the first object to compare
                 * @param two the second object to compare
                 * @return true if (wordId,ctxId) of one is smaller than (wordId,ctxId) of two, otherwise false
                 */
                inline bool operator<(const TCtxIdProbData & one, const TCtxIdProbData & two) {
                    const TLongId key1 = TShortId_TShortId_2_TLongId(one.wordId, one.ctxId);
                    const TLongId key2 = TShortId_TShortId_2_TLongId(two.wordId, two.ctxId);
                    return (key1 < key2);
                    /* ToDo: An alternative for testing, which is faster?
                    if (one.wordId < two.wordId) {
                        return true;
                    } else {
                        if (one.wordId > two.wordId) {
                            return false;
                        } else {
                            if (one.ctxId < two.ctxId) {
                                return true;
                            } else {
                                return false;
                            }
                        }
                    }
                     */
                };
            }

            /**
             * This is the Context to word array memory trie implementation class.
             * 
             * WARNING: This trie assumes that the M-grams (1 <= M < N) are added
             * to the Trie in an ordered way and there are no duplicates in the
             * 1-Grams. The order is assumed to be lexicographical as in the ARPA
             * files! This is also checked if the sanity checks are on see Globals.hpp!
             * 
             * @param N the maximum number of levels in the trie.
             */
            template<TModelLevel N>
            class C2WOrderedArrayTrie : public ALayeredTrie<N> {
            public:

                /**
                 * The basic constructor
                 * @param p_word_index the word index (dictionary) container
                 */
                explicit C2WOrderedArrayTrie(AWordIndex * const p_word_index);

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * For more details @see ATrie
                 */
                virtual void preAllocate(const size_t counts[N]);

                /**
                 * The basic destructor
                 */
                virtual ~C2WOrderedArrayTrie();

            protected:

                /**
                 * This structure is needed to store begin and end index to reference pieces of an array
                 * It is used to reference sub-array ranges for the M-gram data for levels 1 < M < N.
                 * @param beginIdx the begin index
                 * @param endIdx the end index
                 */
                typedef struct {
                    TShortId beginIdx;
                    TShortId endIdx;
                } TSubArrReference;

                /**
                 * This structure stores two things the word id
                 * and the corresponding probability/back-off data.
                 * It is used to store the M-gram data for levels 1 < M < N.
                 * @param wordId the word id
                 * @param data the back-off and probability data
                 */
                typedef struct {
                    TShortId wordId;
                    TProbBackOffEntry data;
                } TWordIdProbBackOffEntryPair;

                typedef __C2WOrderedArrayTrie::TCtxIdProbData TCtxIdProbEntry;
                
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
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - ALayeredTrie<N>::MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Adding " << SSTR(level) << "-gram with ctxId: "
                            << SSTR(ctxId) << ", wordId: " << SSTR(wordId) << END_LOG;

                    //First get the sub-array reference. 
                    TSubArrReference & ref = m_M_gram_ctx_2_data[mgram_idx][ctxId];

                    //Check that the array is continuous in indexes, so that we add
                    //context after context and not switching between different contexts!
                    if (DO_SANITY_CHECKS && (ref.endIdx != ALayeredTrie<N>::UNDEFINED_ARR_IDX)
                            && (ref.endIdx + 1 != m_M_N_gram_next_ctx_id[mgram_idx])) {
                        stringstream msg;
                        msg << "The " << SSTR(level) << " -gram ctxId: " << SSTR(ctxId)
                                << " array is not ordered ref.endIdx = " << SSTR(ref.endIdx)
                                << ", next ref.endIdx = " << SSTR(m_M_N_gram_next_ctx_id[mgram_idx] + 1) << "!";
                        throw Exception(msg.str());
                    }

                    //Get the new index and increment - this will be the new end index
                    ref.endIdx = m_M_N_gram_next_ctx_id[mgram_idx]++;

                    //Check if we exceeded the maximum allowed number of M-grams
                    if (DO_SANITY_CHECKS && (ref.endIdx >= m_M_N_gram_num_ctx_ids[mgram_idx])) {
                        stringstream msg;
                        msg << "The maximum allowed number of " << SSTR(level) << "-grams: "
                                << SSTR(m_M_N_gram_num_ctx_ids[mgram_idx]) << " is exceeded )!";
                        throw Exception(msg.str());
                    }

                    //Check if there are yet no elements for this context
                    if (ref.beginIdx == ALayeredTrie<N>::UNDEFINED_ARR_IDX) {
                        //There was no elements put into this contex, the begin index is then equal to the end index
                        ref.beginIdx = ref.endIdx;
                    }

                    //Store the word id
                    m_M_gram_data[mgram_idx][ref.endIdx].wordId = wordId;

                    //Return the reference to the newly allocated element
                    return m_M_gram_data[mgram_idx][ref.endIdx].data;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual bool get_M_GramDataRef(const TModelLevel level, const TShortId wordId,
                        TLongId ctxId, const TProbBackOffEntry **ppData) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - ALayeredTrie<N>::MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the context id, note we use short ids here!
                    if (getContextId(wordId, ctxId, level)) {
                        //Return the data by the context
                        *ppData = &m_M_gram_data[mgram_idx][ctxId].data;
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
                    //Get the new n-gram index
                    const TShortId n_gram_idx = m_M_N_gram_next_ctx_id[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR]++;

                    LOG_DEBUG2 << "Adding " << SSTR(N) << "-gram with ctxId: " << SSTR(ctxId)
                            << ", wordId: " << SSTR(wordId) << " @ index: " << SSTR(n_gram_idx) << END_LOG;

                    //Check if we exceeded the maximum allowed number of M-grams
                    if (DO_SANITY_CHECKS && (n_gram_idx >= m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR])) {
                        stringstream msg;
                        msg << "The maximum allowed number of " << SSTR(N) << "-grams: "
                                << SSTR(m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR]) << " is exceeded )!";
                        throw Exception(msg.str());
                    }

                    //Store the context and word ids
                    m_N_gram_data[n_gram_idx].ctxId = ctxId;
                    m_N_gram_data[n_gram_idx].wordId = wordId;

                    //Create the search key by combining ctx and word ids, see TCtxIdProbEntryPair
                    const TLongId key = TShortId_TShortId_2_TLongId(wordId, ctxId);
                    LOG_DEBUG4 << "Storing N-Gram: TShortId_TShortId_2_TLongId(wordId = " << SSTR(wordId)
                            << ", ctxId = " << SSTR(ctxId) << ") = " << SSTR(key) << END_LOG;

                    //return the reference to the probability
                    return m_N_gram_data[n_gram_idx].prob;
                };

                /**
                 * Allows to retrieve the probability value for the N gram defined by the end wordId and ctxId.
                 * For more details @see ATrie
                 */
                virtual bool get_N_GramProb(const TShortId wordId, const TLongId ctxId,
                        TLogProbBackOff & prob) {
                    LOG_DEBUG2 << "Getting " << SSTR(N) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Create the search key by combining ctx and word ids, see TCtxIdProbEntryPair
                    const TLongId key = TShortId_TShortId_2_TLongId(wordId, ctxId);
                    LOG_DEBUG4 << "Searching N-Gram: TShortId_TShortId_2_TLongId(wordId = " << SSTR(wordId)
                            << ", ctxId = " << SSTR(ctxId) << ") = " << SSTR(key) << END_LOG;

                    //Search for the index using binary search
                    TShortId idx = ALayeredTrie<N>::UNDEFINED_ARR_IDX;
                    if (bsearch_wordId_ctxId<TCtxIdProbEntry>(m_N_gram_data, ALayeredTrie<N>::FIRST_VALID_CTX_ID,
                            m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR], wordId, ctxId, idx)) {
                        //return the reference to the probability
                        prob = m_N_gram_data[idx].prob;
                        return true;
                    } else {
                        LOG_DEBUG1 << "Unable to find " << SSTR(N) << "-gram data for ctxId: "
                                << SSTR(ctxId) << ", wordId: " << SSTR(wordId)
                                << ", key " << SSTR(key) << END_LOG;
                        return false;
                    }
                };

                virtual bool isPost_Grams(const TModelLevel level) {
                    //Check the base class and we need to do post actions
                    //for the N-grams. The N-grams level data has to be
                    //sorted see post_N_Grams method implementation below.
                    return (level == N) || ALayeredTrie<N>::isPost_Grams(level);
                }

                virtual void post_N_Grams() {
                    //Call the base class method first
                    ALayeredTrie<N>::post_N_Grams();

                    LOG_DEBUG2 << "Sorting the N-gram's data: ptr: " << m_N_gram_data
                            << ", size: " << m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR] << END_LOG;

                    //Order the N-gram array as it is unordered and we will binary search it later!
                    //Note: We dot not use Q-sort as it needs quite a lot of extra memory!
                    //Also, I did not yet see any performance advantages compared to sort!
                    //Actually the qsort provided here was 50% slower on a 20 Gb language
                    //model when compared to the str::sort!
                    my_sort<TCtxIdProbEntry>(m_N_gram_data, m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::N_GRAM_IDX_IN_M_N_ARR]);
                };

            private:

                //Stores the 1-gram data
                TProbBackOffEntry * m_1_gram_data;

                //Stores the M-gram context to data mappings for: 1 < M < N
                //This is a two dimensional array
                TSubArrReference * m_M_gram_ctx_2_data[ALayeredTrie<N>::NUM_M_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                TWordIdProbBackOffEntryPair * m_M_gram_data[ALayeredTrie<N>::NUM_M_GRAM_LEVELS];

                //Stores the N-gram data
                TCtxIdProbEntry * m_N_gram_data;

                //Stores the maximum number of context id  per M-gram level: 1 < M <= N
                TShortId m_M_N_gram_num_ctx_ids[ALayeredTrie<N>::NUM_M_N_GRAM_LEVELS];
                //Stores the context id counters per M-gram level: 1 < M <= N
                TShortId m_M_N_gram_next_ctx_id[ALayeredTrie<N>::NUM_M_N_GRAM_LEVELS];

                /**
                 * Computes the N-Gram context using the previous context and the current word id
                 * 
                 * WARNING: Must only be called for the M-gram level 1 < M < N!
                 * 
                 * @param wordId the current word id
                 * @param ctxId [in] - the previous context id, [out] - the next context id
                 * @param level the M-gram level we are working with M
                 * @return the resulting context
                 * @throw nothing
                 */
                inline bool getContextId(const TShortId wordId, TLongId & ctxId, const TModelLevel level) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - ALayeredTrie<N>::MGRAM_IDX_OFFSET;

                    if (DO_SANITY_CHECKS && ((level == N) || (mgram_idx < 0))) {
                        stringstream msg;
                        msg << "Unsupported level id: " << level;
                        throw Exception(msg.str());
                    }

                    LOG_DEBUG2 << "Searching for the next ctxId of " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << ", ctxId: "
                            << SSTR(ctxId) << END_LOG;

                    //First get the sub-array reference. Note that, even if it is the 2-Gram
                    //case and the previous word is unknown (ctxId == 0) we still can use
                    //the ctxId to get the data entry. The reason is that we allocated memory
                    //for it but being for an unknown word context it should have no data!
                    TSubArrReference & ref = m_M_gram_ctx_2_data[mgram_idx][ctxId];

                    LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                            << ", with beginIdx: " << SSTR(ref.beginIdx) << ", endIdx: "
                            << SSTR(ref.endIdx) << END_LOG;

                    //Check that there is data for the given context available
                    if (ref.beginIdx != ALayeredTrie<N>::UNDEFINED_ARR_IDX) {
                        TShortId nextCtxId = ALayeredTrie<N>::UNDEFINED_ARR_IDX;
                        //The data is available search for the word index in the array
                        if (bsearch_wordId<TWordIdProbBackOffEntryPair>(m_M_gram_data[mgram_idx], ref.beginIdx, ref.endIdx, wordId, nextCtxId)) {
                            LOG_DEBUG1 << "The next ctxId for wordId: " << SSTR(wordId) << ", ctxId: "
                                    << SSTR(ctxId) << " is nextCtxId: " << SSTR(nextCtxId) << END_LOG;
                            ctxId = nextCtxId;
                            return true;
                        } else {
                            LOG_DEBUG1 << "Unable to find M-gram ctxId for level: "
                                    << SSTR(level) << ", prev ctxId: " << SSTR(ctxId)
                                    << ", wordId: " << SSTR(wordId) << ", is not in the available range: ["
                                    << SSTR(m_M_gram_data[mgram_idx][ref.beginIdx].wordId) << " ... "
                                    << SSTR(m_M_gram_data[mgram_idx][ref.endIdx].wordId) << "]" << END_LOG;
                            return false;
                        }
                    } else {
                        LOG_DEBUG1 << "Unable to find M-gram context id for level: "
                                << SSTR(level) << ", prev ctxId: " << SSTR(ctxId)
                                << ", nothing present in that context!" << END_LOG;
                        return false;
                    }
                }
            };
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

