/* 
 * File:   GenericTrieBase.hpp
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
 * Created on September 20, 2015, 5:21 PM
 */

#ifndef GENERICTRIEBASE_HPP
#define	GENERICTRIEBASE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "ModelMGram.hpp"
#include "QueryMGram.hpp"
#include "TextPieceReader.hpp"

#include "BasicWordIndex.hpp"
#include "CountingWordIndex.hpp"
#include "OptimizingWordIndex.hpp"

#include "WordIndexTrieBase.hpp"

#include "BitmapHashCache.hpp"

using namespace std;
using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;
using namespace uva::utils::math::bits;
using namespace uva::smt::tries::caching;

namespace uva {
    namespace smt {
        namespace tries {

            //This macro is needed to report the collision detection warnings!
#define REPORT_COLLISION_WARNING(gram, word_id, contextId, prevProb, prevBackOff, newProb, newBackOff)   \
            LOG_WARNING << "The " << gram.get_m_gram_level() << "-Gram : " << (string) gram              \
                        << " has been already seen! Word Id: " << SSTR(word_id)                          \
                        << ", context Id: " << SSTR(contextId) << ". "                                   \
                        << "Changing the (prob,back-off) data from ("                                    \
                        << prevProb << "," << prevBackOff << ") to ("                                    \
                        << newProb << "," << newBackOff << ")" << END_LOG;

            /**
             * Contains the m-gram status values:
             * 0. UNDEFINED_MGS - the status is undefined
             * 1. BAD_END_WORD_UNKNOWN_MGS - the m-gram is definitely not present the end word is unknown
             * 2. BAD_NO_PAYLOAD_MGS - the m-gram is definitely not present, the m-gram hash is not cached,
             *                         or it is not found in the trie (the meaning depends on the context)
             * 3. GOOD_PRESENT_MGS   - the m-gram is potentially present, its hash is cached, or it is
             *                         found in the trie (the meaning depends on the context)
             */
            enum MGramStatusEnum {
                UNDEFINED_MGS = 0, BAD_END_WORD_UNKNOWN_MGS = 1, BAD_NO_PAYLOAD_MGS = 2, GOOD_PRESENT_MGS = 3
            };
            static const char* MGramStatusEnumStrings[] = {"UNDEFINED_MGS", "BAD_END_WORD_UNKNOWN_MGS", "BAD_NO_PAYLOAD_MGS", "GOOD_PRESENT_MGS"};

            /**
             * Allows to convert the status enumeration value to its literal representation
             * @param value the status value
             * @return the constant string representation of the value
             */
            static inline const char * status_to_string(const MGramStatusEnum value) {
                return MGramStatusEnumStrings[value];
            }

            /**
             * This class defined the trie interface and functionality that is expected by the TrieDriver class
             */
            template<typename TrieType, TModelLevel MAX_LEVEL, typename WordIndexType, bool NEEDS_BITMAP_HASH_CACHE>
            class GenericTrieBase : public WordIndexTrieBase<MAX_LEVEL, WordIndexType> {
            public:
                //Typedef the base class
                typedef WordIndexTrieBase<MAX_LEVEL, WordIndexType> BASE;

                /**
                 * This structure stores the basic data required for a query execution.
                 * @param m_query the m-gram query itself 
                 * @param m_payloads the  two dimensional array of the payloads 
                 * @param m_last_ctx_ids stores the last context id computed for the given row of the sub-m-gram matrix
                 * @param m_probs the array f probabilities 
                 * @param m_begin_word_idx the currently considered begin word index
                 * @param m_end_word_idx the currently considered end word index
                 */
                struct S_Query_Exec_Data {
                    T_Query_M_Gram<WordIndexType> m_gram;
                    const void * m_payloads[MAX_LEVEL][MAX_LEVEL];
                    TLongId m_last_ctx_ids[MAX_LEVEL];
                    TLogProbBackOff m_probs[MAX_LEVEL];
                    TModelLevel m_begin_word_idx;
                    TModelLevel m_end_word_idx;

                    explicit S_Query_Exec_Data(WordIndexType & word_index) : m_gram(word_index) {
                    }
                };
                typedef S_Query_Exec_Data T_Query_Exec_Data;

                //The offset, relative to the M-gram level M for the m-gram mapping array index
                const static TModelLevel MGRAM_IDX_OFFSET = 2;

                //Will store the the number of M levels such that 1 < M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = MAX_LEVEL - MGRAM_IDX_OFFSET;

                //Will store the the number of M levels such that 1 < M <= N.
                const static TModelLevel NUM_M_N_GRAM_LEVELS = MAX_LEVEL - 1;

                //Compute the N-gram index in in the arrays for M and N grams
                static const TModelLevel N_GRAM_IDX_IN_M_N_ARR = MAX_LEVEL - MGRAM_IDX_OFFSET;

                // Stores the undefined index array value
                static const TShortId UNDEFINED_ARR_IDX = 0;

                // Stores the undefined index array value
                static const TShortId FIRST_VALID_CTX_ID = UNDEFINED_ARR_IDX + 1;

                /**
                 * The basic constructor
                 * @param word_index the word index to be used
                 */
                explicit GenericTrieBase(WordIndexType & word_index)
                : WordIndexTrieBase<MAX_LEVEL, WordIndexType> (word_index) {
                    ASSERT_CONDITION_THROW((MAX_LEVEL> MAX_SUPP_GRAM_LEVEL), string("Unsupported max level: ") + 
                            std::to_string(MAX_LEVEL) + string(", the maximum supported is: ") + std::to_string(MAX_SUPP_GRAM_LEVEL));
                }

                /**
                 * Allows to indicate whether the context id of an m-gram is to be computed while retrieving payloads
                 * @return returns false, by default all generic tries need NO context ids when searching for data
                 */
                static constexpr bool is_need_getting_ctx_ids() {
                    return false;
                }

                /**
                 * @see WordIndexTrieBase
                 */
                inline void pre_allocate(const size_t counts[MAX_LEVEL]) {
                    BASE::pre_allocate(counts);

                    //Pre-allocate the bitmap cache for hashes if needed
                    if (NEEDS_BITMAP_HASH_CACHE) {
                        for (size_t idx = 0; idx < NUM_M_N_GRAM_LEVELS; ++idx) {
                            m_bitmap_hash_cach[idx].pre_allocate(counts[idx + 1]);
                            Logger::update_progress_bar();
                        }
                    }
                }

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @param gram the M-Gram data
                 * @throws Exception if the level of this M-gram is not such that  1 < M < N
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    THROW_MUST_OVERRIDE();
                };

                /**
                 * Allows to check if the given sub-m-gram, defined by the begin_word_idx
                 * and end_word_idx parameters, is potentially present in the trie.
                 * THis method must not be called for uni-grams, those always have a payload!
                 * @param query the m-gram query data
                 * @param status [out] the resulting status of the operation
                 */
                inline void is_m_gram_potentially_present(const T_Query_Exec_Data& query,
                        MGramStatusEnum &status) const {
                    //Do sanity check if needed
                    ASSERT_SANITY_THROW((query.m_begin_word_idx != query.m_end_word_idx), "Trying the check the bitmap hash cache for a uni-gram!");

                    //Check if the caching is enabled and needed
                    if (NEEDS_BITMAP_HASH_CACHE) {
                        //Check if the end word is unknown, if not proceed to the cache check
                        if (query.m_gram[query.m_end_word_idx] != WordIndexType::UNKNOWN_WORD_ID) {
                            //Check if the begin word is unknown, if not proceed to the cache check
                            if (query.m_gram[query.m_begin_word_idx] != WordIndexType::UNKNOWN_WORD_ID) {
                                //Compute the level array index
                                const TModelLevel level_idx = CURR_LEVEL_MIN_2_MAP[query.m_begin_word_idx][query.m_end_word_idx];

                                //If the caching is enabled, the higher sub-m-gram levels always require checking
                                const BitmapHashCache & ref = m_bitmap_hash_cach[level_idx];

                                //Get the m-gram's hash
                                const uint64_t hash = query.m_gram.template get_hash(query.m_begin_word_idx, query.m_end_word_idx);

                                if (ref.is_hash_cached(hash)) {
                                    //The m-gram hash is cached, so potentially a payload data
                                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                                } else {
                                    //The m-gram hash is not in cache, so definitely no data
                                    status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                                }
                            } else {
                                //The m-gram hash is not in cache, so definitely no data
                                status = MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                            }
                        } else {
                            //The end word is unknown, so definitely no m-gram data
                            status = MGramStatusEnum::BAD_END_WORD_UNKNOWN_MGS;
                        }
                    } else {
                        //If caching is not enabled then we always check the trie
                        status = MGramStatusEnum::GOOD_PRESENT_MGS;
                    }
                }

                /**
                 * This method allows to get the payloads and compute the (joint) m-gram probabilities.
                 * @param TrieType the trie type
                 * @param DO_JOINT_PROBS true if we want joint probabilities per sum-m-gram, otherwise false (one conditional m-gram probability)
                 * @param trie the trie instance reference
                 * @param query the query execution data for storing the query, and retrieved payloads, and resulting probabilities, and etc.
                 */
                template<bool DO_JOINT_PROBS>
                inline void execute(T_Query_Exec_Data & query) const {
                    //Declare the stream-compute result status variable
                    MGramStatusEnum status = MGramStatusEnum::GOOD_PRESENT_MGS;

                    //Initialize the begin and end index variables
                    query.m_begin_word_idx = query.m_gram.get_begin_word_idx();
                    //Check if we need cumulative or single conditional m-gram probability
                    query.m_end_word_idx = (DO_JOINT_PROBS) ? query.m_begin_word_idx : query.m_gram.get_end_word_idx();

                    //If this is at least a bi-gram, continue iterations, otherwise we are done!
                    while (query.m_end_word_idx <= query.m_gram.get_end_word_idx()) {
                        LOG_DEBUG << "-----> Streaming cumulative sub-m-gram [" << SSTR(query.m_begin_word_idx)
                                << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;
                        //Check whether this is a uni-gram case
                        if (query.m_begin_word_idx != query.m_end_word_idx) {
                            //If this is not a uni-gram case then 
                            switch (status) {
                                case MGramStatusEnum::BAD_NO_PAYLOAD_MGS: //The previous status was: unknown payload
                                {
                                    //The m-gram payload was not found, so this is not a uni-gram. The method will
                                    //increment the begin index to move to the next row, the column remains.
                                    process_unknown(query);

                                    //The status is always good as back-off can not fail, even if the back-off payload
                                    //is not found, we then just count as if the back-off weight is zero and move on to
                                    //computing the next row probability.
                                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                                    break;
                                }
                                case MGramStatusEnum::BAD_END_WORD_UNKNOWN_MGS: //The previous status was: unknown end word
                                {
                                    //The m-gram end word was not found, so this is not a uni-gram The method will 
                                    //increment the begin index to move to the next row, while the column remains.
                                    process_unknown(query);

                                    //The status is always an unknown end word, so that we keep iterating down the column
                                    status = MGramStatusEnum::BAD_END_WORD_UNKNOWN_MGS;
                                    break;
                                }
                                case MGramStatusEnum::GOOD_PRESENT_MGS: //The previous status was good: process the next sub-m-gram
                                {
                                    //Retrieve the next sub-m-gram payload
                                    process_m_n_gram(query, status);
                                    //If the sub-m-gram was found then just move on
                                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                        //Move on to the longer sub-m-gram
                                        ++query.m_end_word_idx;
                                    }
                                    break;
                                }
                                default:
                                {
                                    THROW_EXCEPTION(string("Unsupported status value: ") + status_to_string(status));
                                }
                            }
                        } else {
                            //Retrieve the payload
                            process_unigram(query, status);
                            //Move on to the longer sub-m-gram
                            ++query.m_end_word_idx;
                        }
                    }
                };

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success.
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_unigram_payload(T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    THROW_MUST_NOT_CALL();
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for 1<m<n
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_m_gram_payload(T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    THROW_MUST_NOT_CALL();
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==n
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_n_gram_payload(T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    THROW_MUST_NOT_CALL();
                }

                /**
                 * Is to be used from the sub-classes from the add_X_gram methods.
                 * This method allows to register the given M-gram in internal high
                 * level caches if present.
                 * 
                 * WARNING: Is not to be used on uni-grams!!!
                 * 
                 * @param gram the M-gram to cache
                 */
                inline void register_m_gram_cache(const T_Model_M_Gram<WordIndexType> &gram) {
                    if (NEEDS_BITMAP_HASH_CACHE) {
                        const TModelLevel curr_level = gram.get_m_gram_level();
                        ASSERT_SANITY_THROW((curr_level == M_GRAM_LEVEL_1), "Trying to add a uni-gram to a bitmap hash cache!");
                        m_bitmap_hash_cach[curr_level - MGRAM_IDX_OFFSET].template cache_m_gram_hash<WordIndexType>(gram);
                    }
                }

                /**
                 * The basic class destructor
                 */
                virtual ~GenericTrieBase() {
                }

            private:

                //Stores the bitmap hash caches per M-gram level for 1 < M <= N
                BitmapHashCache m_bitmap_hash_cach[NUM_M_N_GRAM_LEVELS];

                /**
                 * This method allows to process the uni-gram case, retrieve payload and account for probabilities.
                 * @param TrieType the trie type
                 * @param query the m-gram query data
                 * @param status [out] the resulting status of the operation, will always be MGramStatusEnum::GOOD_PRESENT_MGS
                 */
                inline void process_unigram(T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //Get the uni-gram word index
                    const TModelLevel & word_idx = query.m_begin_word_idx;
                    //Get the reference to the payload for convenience
                    const void * & payload = query.m_payloads[word_idx][word_idx];

                    //Retrieve the payload
                    static_cast<const TrieType*> (this)->get_unigram_payload(query);
                    LOG_DEBUG << "The 1-gram is found, payload: "
                            << (string) (* (const T_M_Gram_Payload *) payload) << END_LOG;

                    //No need to check on the status, it is always good for the uni-gram
                    query.m_probs[word_idx] += ((const T_M_Gram_Payload *) payload)->m_prob;
                    LOG_DEBUG << "probs[" << SSTR(word_idx) << "] += "
                            << ((const T_M_Gram_Payload *) payload)->m_prob << END_LOG;

                    //The payload of a uni-gram is always present, even if
                    //it is an unknown word, the data is still available.
                    status = MGramStatusEnum::GOOD_PRESENT_MGS;
                }

                /**
                 * This method allows to process the probability for the m/n-gram defined
                 * by the method arguments.
                 * @param query the m-gram query data
                 * @param status the resulting status of computations
                 */
                inline void process_m_n_gram(T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    //If this is at least a bi-gram, continue iterations, otherwise we are done!
                    LOG_DEBUG << "Considering the sub-m-gram: [" << SSTR(query.m_begin_word_idx) << "," << SSTR(query.m_end_word_idx) << "]" << END_LOG;

                    //First check if it makes sense to look into  the trie
                    is_m_gram_potentially_present(query, status);
                    LOG_DEBUG << "The payload availability status for sub-m-gram : [" << SSTR(query.m_begin_word_idx) << ","
                            << SSTR(query.m_end_word_idx) << "] is: " << status_to_string(status) << END_LOG;

                    //If the status says that the m-gram is potentially present then we try to retrieve it from the trie
                    if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                        //Compute the current sub-m-gram level
                        const TModelLevel curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];
                        LOG_DEBUG << "The current sub-m-gram level is: " << SSTR(curr_level) << END_LOG;

                        //Just for convenience get the reference to the payload element
                        const void * & payload_elem = query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx];

                        //Obtain the payload, depending on the sub-m-gram level
                        if (curr_level == MAX_LEVEL) {
                            LOG_DEBUG << "Calling the get_" << SSTR(curr_level) << "_gram_payload function." << END_LOG;
                            //We are at the last trie level, retrieve the payload
                            static_cast<const TrieType*> (this)->get_n_gram_payload(query, status);

                            //Append the probability if the retrieval was successful
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                LOG_DEBUG << "The n-gram is found, probability: "
                                        << *((const TLogProbBackOff *) payload_elem) << END_LOG;
                                query.m_probs[query.m_end_word_idx] += *((const TLogProbBackOff *) payload_elem);
                                LOG_DEBUG << "probs[" << SSTR(query.m_begin_word_idx) << "] += "
                                        << *((const TLogProbBackOff *) payload_elem) << END_LOG;
                            }
                        } else {
                            LOG_DEBUG << "Calling the get_" << SSTR(curr_level) << "_gram_payload function." << END_LOG;
                            //We are at one of the intermediate trie level, retrieve the payload
                            static_cast<const TrieType*> (this)->get_m_gram_payload(query, status);

                            //Append the probability if the retrieval was successful
                            if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                                LOG_DEBUG << "The m-gram is found, payload: "
                                        << (string) (* ((const T_M_Gram_Payload *) payload_elem)) << END_LOG;
                                query.m_probs[query.m_end_word_idx] += ((const T_M_Gram_Payload *) payload_elem)->m_prob;
                                LOG_DEBUG << "probs[" << SSTR(query.m_begin_word_idx) << "] += "
                                        << ((const T_M_Gram_Payload *) payload_elem)->m_prob << END_LOG;
                            }
                        }
                    }

                    LOG_DEBUG << "The result for the sub-m-gram: [" << SSTR(query.m_begin_word_idx) << ","
                            << SSTR(query.m_end_word_idx) << "] is : " << status_to_string(status) << END_LOG;
                }

                /**
                 * This method allows to retrieve the payload of a uni-gram or an m-gram with m < n
                 * @param query the m-gram query data
                 * @return the resulting status of the operation
                 */
                inline MGramStatusEnum get_uni_m_gram_payload(T_Query_Exec_Data & query) const {
                    LOG_DEBUG << "The payload for sub-m-gram : [" << SSTR(query.m_begin_word_idx) << ","
                            << SSTR(query.m_end_word_idx) << "] needs to be retrieved!" << END_LOG;
                    //Try to retrieve the back-off sub-m-gram
                    if (query.m_begin_word_idx == query.m_end_word_idx) {
                        //If the back-off sub-m-gram is a uni-gram then
                        static_cast<const TrieType*> (this)->get_unigram_payload(query);

                        //Set the result status to good
                        return MGramStatusEnum::GOOD_PRESENT_MGS;
                    } else {
                        //Declare the status variable
                        MGramStatusEnum status;

                        //Check if the back-off sub-m-gram is potentially available
                        is_m_gram_potentially_present(query, status);
                        LOG_DEBUG << "The payload availability status for sub-m-gram : [" << SSTR(query.m_begin_word_idx) << ","
                                << SSTR(query.m_end_word_idx) << "] is: " << status_to_string(status) << END_LOG;

                        if (status == MGramStatusEnum::GOOD_PRESENT_MGS) {
                            //The back-off sub-m-gram has a level M: 1 < M < N
                            static_cast<const TrieType*> (this)->get_m_gram_payload(query, status);
                        }

                        return status;
                    }
                }

                /**
                 * This method adds the back-off weight of the given m-gram, if it is to be found in the trie
                 * @param query the m-gram query data the begin word index will be changed
                 */
                inline void process_unknown(T_Query_Exec_Data & query) const {
                    LOG_DEBUG << "query.m_begin_word_idx = " << SSTR(query.m_begin_word_idx) << ","
                            << " query.m_end_word_idx = " << SSTR(query.m_end_word_idx) << END_LOG;

                    //Decrease the end word index, as we need the back-off data
                    query.m_end_word_idx--;

                    //Define the reference to the payload pointer, just for convenience
                    const void * & bo_payload_ref = query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx];

                    //If the back-off payload is present, then take it into account, else try to retrieve it and take into account
                    if ((bo_payload_ref != NULL) || (get_uni_m_gram_payload(query) == MGramStatusEnum::GOOD_PRESENT_MGS)) {
                        LOG_DEBUG << "The m-gram is found, payload: "
                                << (string) (* ((const T_M_Gram_Payload *) bo_payload_ref)) << END_LOG;
                        query.m_probs[query.m_end_word_idx + 1] += ((const T_M_Gram_Payload *) bo_payload_ref)->m_back;
                        LOG_DEBUG << "probs[" << SSTR(query.m_end_word_idx + 1) << "] += "
                                << ((const T_M_Gram_Payload *) bo_payload_ref)->m_back << END_LOG;
                    }

                    //Increase the end word index as we are going back to normal
                    query.m_end_word_idx++;

                    //Next we shift to the next row, it is possible as it is not a uni-gram case, the latter 
                    //always get MGramStatusEnum::GOOD_PRESENT_MGS result when their payload is retrieved
                    query.m_begin_word_idx++;

                    LOG_DEBUG << "query.m_begin_word_idx = " << SSTR(query.m_begin_word_idx) << ","
                            << " query.m_end_word_idx = " << SSTR(query.m_end_word_idx) << END_LOG;
                }
            };

            //Define the macro for instantiating the generic trie class children templates
#define INSTANTIATE_TRIE_FUNCS_LEVEL(LEVEL, TRIE_TYPE_NAME, ...) \
            template void TRIE_TYPE_NAME<__VA_ARGS__>::add_m_gram<LEVEL>(const T_Model_M_Gram<TRIE_TYPE_NAME<__VA_ARGS__>::WordIndexType> & gram);

#define INSTANTIATE_TRIE_TEMPLATE_TYPE(TRIE_TYPE_NAME, ...) \
            template class TRIE_TYPE_NAME<__VA_ARGS__>; \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_1, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_2, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_3, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_4, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_5, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_6, TRIE_TYPE_NAME, __VA_ARGS__); \
            INSTANTIATE_TRIE_FUNCS_LEVEL(M_GRAM_LEVEL_7, TRIE_TYPE_NAME, __VA_ARGS__);

        }
    }
}


#endif	/* GENERICTRIEBASE_HPP */

