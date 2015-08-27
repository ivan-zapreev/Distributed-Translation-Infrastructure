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
                 * This structure is needed to store begin and end index to reference pieces of an array
                 * It is used to reference sub-array ranges for the M-gram data for levels 1 < M <= N.
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
                 * @param ctxId the context id
                 * @param data the back-off and probability data
                 */
                typedef struct {
                    TShortId ctxId;
                    TProbBackOffEntryPair data;

                    operator TShortId() const {
                        return ctxId;
                    }
                } TCtxIdProbBackOffEntryPair;

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
                } TCtxIdProbEntryPair;

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair & make_1_GramDataRef(const TShortId wordId) {
                    LOG_DEBUG2 << "Adding 1-gram with wordId: " << SSTR(wordId) << END_LOG;
                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the One gram with the given Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TProbBackOffEntryPair & get_1_GramDataRef(const TShortId wordId) {
                    LOG_DEBUG2 << "Getting 1-gram with wordId: " << SSTR(wordId) << END_LOG;

                    return m_1_gram_data[wordId];
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * This method words for 1 < M <= N
                 */
                virtual TSubArrReference & make_M_N_GramEntry(const TModelLevel & level, const TShortId & wordId) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Making entry for \t" << SSTR(level) << "-gram with wordId:\t" << SSTR(wordId) << END_LOG;

                    //First get the sub-array reference. 
                    TSubArrReference & ref = m_M_N_gram_word_2_data[mgram_idx][wordId];

                    //Check that the array is continuous in indexes, so that we add
                    //context after context and not switching between different contexts!
                    if (DO_SANITY_CHECKS && (ref.endIdx != UNDEFINED_ARR_IDX) && (ref.endIdx + 1 != m_M_N_gram_next_ctx_id[mgram_idx])) {
                        stringstream msg;
                        msg << "The " << SSTR(level) << " -gram wordId: " << SSTR(wordId)
                                << " array is not ordered ref.endIdx = " << SSTR(ref.endIdx)
                                << ", next ref.endIdx = " << SSTR(m_M_N_gram_next_ctx_id[mgram_idx] + 1) << "!";
                        throw Exception(msg.str());
                    }

                    //Get the new index and increment - this will be the new end index
                    ref.endIdx = m_M_N_gram_next_ctx_id[mgram_idx]++;

                    //Check if we exceeded the maximum allowed number of M-grams
                    if (DO_SANITY_CHECKS && (ref.endIdx >= m_M_N_gram_num_ctx_ids[mgram_idx])) {
                        stringstream msg;
                        msg << "The maximum allowed number of " << SSTR(level)
                                << "-grams: " << SSTR(m_M_N_gram_num_ctx_ids[mgram_idx])
                                << " is exceeded )!";
                        throw Exception(msg.str());
                    }

                    //Check if there are yet no elements for this context
                    if (ref.beginIdx == UNDEFINED_ARR_IDX) {
                        //There was no elements put into this contex, the begin index is then equal to the end index
                        ref.beginIdx = ref.endIdx;
                    }

                    return ref;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TSubArrReference& get_M_N_GramEntry(const TModelLevel & level, const TShortId & wordId) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Getting sub arr data for " << SSTR(level)
                            << "-gram with wordId: " << SSTR(wordId) << END_LOG;

                    //First get the sub-array reference. 
                    const TSubArrReference & ref = m_M_N_gram_word_2_data[mgram_idx][wordId];

                    //Check if there are elements for this context
                    if (ref.beginIdx != UNDEFINED_ARR_IDX) {
                        return ref;
                    } else {
                        LOG_DEBUG1 << "There are no elements @ level: " << SSTR(level)
                                << " for wordId: " << SSTR(wordId) << "!" << END_LOG;
                        throw out_of_range("not found");
                    }
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TProbBackOffEntryPair& make_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Adding\t" << SSTR(level) << "-gram with ctxId:\t"
                            << SSTR(ctxId) << ", wordId:\t" << SSTR(wordId) << END_LOG;

                    //First get the sub-array reference. 
                    TSubArrReference & ref = make_M_N_GramEntry(level, wordId);

                    //Store the word id
                    m_M_gram_data[mgram_idx][ref.endIdx].ctxId = ctxId;

                    //Return the reference to the newly allocated element
                    return m_M_gram_data[mgram_idx][ref.endIdx].data;
                };

                /**
                 * Allows to retrieve the data storage structure for the M gram
                 * with the given M-gram level Id. M-gram context and last word Id.
                 * If the storage structure does not exist, throws an exception.
                 * For more details @see ATrie
                 */
                virtual const TProbBackOffEntryPair& get_M_GramDataRef(const TModelLevel level, const TShortId wordId, const TLongId ctxId) {
                    //Compute the m-gram index
                    const TModelLevel mgram_idx = level - MGRAM_IDX_OFFSET;

                    LOG_DEBUG2 << "Getting " << SSTR(level) << "-gram with wordId: "
                            << SSTR(wordId) << ", ctxId: " << SSTR(ctxId) << END_LOG;

                    //Get the context id, note we use short ids here!
                    TShortId nextCtxId = (TShortId) getContextId(wordId, ctxId, N);

                    //Return the data by the context
                    return m_M_gram_data[mgram_idx][nextCtxId].data;
                };

                /**
                 * Allows to retrieve the data storage structure for the N gram.
                 * Given the N-gram context and last word Id.
                 * If the storage structure does not exist, return a new one.
                 * For more details @see ATrie
                 */
                virtual TLogProbBackOff& make_N_GramDataRef(const TShortId wordId, const TLongId ctxId) {
                    //Get the new n-gram index
                    const TShortId n_gram_idx = m_M_N_gram_next_ctx_id[N_GRAM_IDX]++;

                    LOG_DEBUG2 << "Adding\t" << SSTR(N) << "-gram with ctxId:\t" << SSTR(ctxId)
                            << ", wordId:\t" << SSTR(wordId) << " @ index:\t" << SSTR(n_gram_idx) << END_LOG;

                    //First get the sub-array reference. 
                    TSubArrReference & ref = make_M_N_GramEntry(N, wordId);

                    //Store the context and word ids
                    m_N_gram_data[ref.endIdx].ctxId = ctxId;

                    //return the reference to the probability
                    return m_N_gram_data[ref.endIdx].prob;
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

                    //Get the context id, note we use short ids here!
                    TShortId nextCtxId = (TShortId) getContextId(wordId, ctxId, N);

                    //Return the data by the context
                    return m_N_gram_data[nextCtxId].prob;
                };

                virtual bool isPost_Grams(const TModelLevel level) {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per word by context id, see
                    //post_M_Grams, and post_N_Grams methods below.
                    return (level > MIN_NGRAM_LEVEL) || ATrie<N>::isPost_Grams(level);
                }

                virtual void post_M_Grams(const TModelLevel level) {
                    //Call the base class method first
                    ATrie<N>::post_N_Grams();

                    //ToDo: Implement sorting by context Id of the data per word id
                }

                virtual void post_N_Grams() {
                    //Call the base class method first
                    ATrie<N>::post_N_Grams();

                    //Order the N-gram array as it is unordered and we will binary search it later!
                    //Note: We do not use qsort as it has worse performance than this method.
                    sort<TCtxIdProbEntryPair, TLongId>(m_N_gram_data, m_N_gram_data + m_M_N_gram_num_ctx_ids[N_GRAM_IDX]);
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
                TProbBackOffEntryPair * m_1_gram_data;

                //Stores the M-gram context to data mappings for: 1 < M <= N
                //This is a two dimensional array
                TSubArrReference * m_M_N_gram_word_2_data[NUM_M_N_GRAM_LEVELS];
                //Stores the M-gram data for the M levels: 1 < M < N
                //This is a two dimensional array
                TCtxIdProbBackOffEntryPair * m_M_gram_data[NUM_M_GRAM_LEVELS];

                //Stores the N-gram data
                TCtxIdProbEntryPair * m_N_gram_data;

                //Stores the maximum number of context id  per M-gram level: 1 < M <= N
                TShortId m_M_N_gram_num_ctx_ids[NUM_M_N_GRAM_LEVELS];
                //Stores the context id counters per M-gram level: 1 < M <= N
                TShortId m_M_N_gram_next_ctx_id[NUM_M_N_GRAM_LEVELS];

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
                    TSubArrReference & ref = m_M_N_gram_word_2_data[mgram_idx][ctxId];

                    LOG_DEBUG2 << "Got context mapping for ctxId: " << SSTR(ctxId)
                            << ", with beginIdx: " << SSTR(ref.beginIdx) << ", endIdx: "
                            << SSTR(ref.endIdx) << END_LOG;

                    //Check that there is data for the given context available
                    if (ref.beginIdx != UNDEFINED_ARR_IDX) {
                        TShortId nextCtxId = UNDEFINED_ARR_IDX;
                        //The data is available search for the word index in the array
                        if (binarySearch<TCtxIdProbBackOffEntryPair, TShortId, TShortId>(m_M_gram_data[mgram_idx], ref.beginIdx, ref.endIdx, wordId, nextCtxId)) {
                            return nextCtxId;
                        } else {
                            LOG_DEBUG1 << "Unable to find M-gram context id for level: "
                                    << SSTR(level) << ", wordId: " << SSTR(wordId)
                                    << ", prev ctxId: " << SSTR(ctxId) << ", ctxId range: ["
                                    << SSTR(m_M_gram_data[mgram_idx][ref.beginIdx].ctxId)
                                    << ", " << SSTR(m_M_gram_data[mgram_idx][ref.endIdx].ctxId) << "]" << END_LOG;
                            throw out_of_range("not found");
                        }
                    } else {
                        LOG_DEBUG1 << "Unable to find M-gram word id data for level: "
                                << SSTR(level) << ", wordId: " << SSTR(wordId)
                                << ", nothing is present!" << END_LOG;
                        throw out_of_range("not found");
                    }
                }
            };

            typedef W2COrderedArrayTrie<MAX_NGRAM_LEVEL> TW2COrderedArrayTrie_N5;
        }
    }
}


#endif	/* CONTEXTTOWORDHYBRIDMEMORYTRIE_HPP */

