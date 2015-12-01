/* 
 * File:   G2DMapTrie.hpp
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
 * Created on September 3, 2015, 3:32 PM
 */

#ifndef G2DHASHMAPTRIE_HPP
#define	G2DHASHMAPTRIE_HPP

#include <string>       // std::string

#include "Globals.hpp"
#include "Exceptions.hpp"

#include "AWordIndex.hpp"
#include "HashingWordIndex.hpp"
#include "ModelMGram.hpp"
#include "ByteMGramId.hpp"

#include "TextPieceReader.hpp"

#include "ArrayUtils.hpp"
#include "DynamicMemoryArrays.hpp"

#include "GenericTrieBase.hpp"

using namespace std;

using namespace uva::smt::logging;
using namespace uva::smt::file;
using namespace uva::smt::tries::alloc;
using namespace uva::smt::tries::dictionary;
using namespace uva::smt::tries::m_grams;

namespace uva {
    namespace smt {
        namespace tries {

            namespace __G2DMapTrie {

#pragma pack(push, 1) // exact fit - no padding

                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 * @param id stores the M-gram id
                 * @param payload stores the payload which is either probability or probability with back-off
                 */
                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    M_GRAM_ID_TYPE id;
                    PAYLOAD_TYPE payload;

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;

                    typedef M_GRAM_ID_TYPE TIdType;
                    typedef PAYLOAD_TYPE TPayloadType;
                };

                typedef S_M_GramData<T_Gram_Id_Data_Ptr, T_M_Gram_Payload> T_M_Gram_PB_Entry;
                typedef S_M_GramData<T_Gram_Id_Data_Ptr, TLogProbBackOff> T_M_Gram_Prob_Entry;

                template<typename ELEMENT_TYPE>
                void destroy_Comp_M_Gram_Id(ELEMENT_TYPE & elem) {
                    m_gram_id::destroy(elem.id);
                };

                template void destroy_Comp_M_Gram_Id<T_M_Gram_PB_Entry>(T_M_Gram_PB_Entry &elem);
                template void destroy_Comp_M_Gram_Id<T_M_Gram_Prob_Entry>(T_M_Gram_Prob_Entry &elem);

                template<typename M_GRAM_ID_TYPE, typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<M_GRAM_ID_TYPE, PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__G2DMapTrie::MEM_INC_TYPE,
                        __G2DMapTrie::MIN_MEM_INC_NUM, __G2DMapTrie::MEM_INC_FACTOR);
            }
#pragma pack(pop) //back to whatever the previous packing mode was 

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param MAX_LEVEL - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class G2DMapTrie : public GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::DO_BITMAP_HASH_CACHE> BASE;
                typedef Byte_M_Gram_Id<typename WordIndexType::TWordIdType, MAX_LEVEL> TM_Gram_Id;
                typedef typename WordIndexType::TWordIdType TWordIdType;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit G2DMapTrie(WordIndexType & word_index);

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO << "Using the #buckets factor: "
                            << SSTR(__G2DMapTrie::BUCKETS_FACTOR) << END_LOG;
                    LOG_INFO << "Using  and the " << __G2DMapTrie::T_M_Gram_PB_Entry::m_mem_strat.get_strategy_info()
                            << " memory allocation strategy." << END_LOG;
                }

                /**
                 * This method can be used to provide the N-gram count information
                 * That should allow for pre-allocation of the memory
                 * @see GenericTrieBase
                 */
                virtual void pre_allocate(const size_t counts[MAX_LEVEL]);

                /**
                 * This method adds a M-Gram (word) to the trie where 1 < M < N
                 * @see GenericTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void add_m_gram(const T_Model_M_Gram<WordIndexType> & gram) {
                    if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                        //Get the word id of this unigram, so there is just one word in it and its the end one
                        const TShortId word_id = gram.get_end_word_id();
                        //Store the probability data in the one gram data storage, under its id
                        m_1_gram_data[word_id] = gram.m_payload;
                    } else {
                        //Register the m-gram in the hash cache
                        this->register_m_gram_cache(gram);

                        //Get the bucket index
                        LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << END_LOG;
                        uint32_t bucket_idx = get_bucket_id(gram.get_hash(), m_bucket_dividers[CURR_LEVEL - 1]);

                        if (CURR_LEVEL == MAX_LEVEL) {
                            //Create a new M-Gram data entry
                            __G2DMapTrie::T_M_Gram_Prob_Entry & data = m_N_gram_data[bucket_idx].allocate();
                            //Create the N-gram id from the word ids
                            gram.create_m_gram_id(gram.get_begin_word_idx(), CURR_LEVEL, data.id);
                            //Set the probability data
                            data.payload = gram.m_payload.m_prob;
                        } else {
                            //Compute the M-gram level index
                            constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                            //Create a new M-Gram data entry
                            __G2DMapTrie::T_M_Gram_PB_Entry & data = m_M_gram_data[LEVEL_IDX][bucket_idx].allocate();
                            //Create the M-gram id from the word ids.
                            gram.create_m_gram_id(gram.get_begin_word_idx(), CURR_LEVEL, data.id);
                            //Set the probability and back-off data
                            data.payload = gram.m_payload;
                        }
                    }
                }

                /**
                 * This method allows to check if post processing should be called after
                 * all the X level grams are read. This method is virtual.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                bool is_post_grams() const {
                    //Check the base class and we need to do post actions
                    //for all the M-grams with 1 < M <= N. The M-grams level
                    //data has to be ordered per bucket per id, see
                    //post_M_Grams, and post_N_Grams methods below.

                    return (CURR_LEVEL > M_GRAM_LEVEL_1) || BASE::template is_post_grams<CURR_LEVEL>();
                }

                /**
                 * This method should be called after all the X level grams are read.
                 * For more details @see WordIndexTrieBase
                 */
                template<TModelLevel CURR_LEVEL>
                inline void post_grams() {
                    //Call the base class method first
                    if (BASE::template is_post_grams<CURR_LEVEL>()) {
                        BASE::template post_grams<CURR_LEVEL>();
                    }

                    //Do the post actions here
                    if (CURR_LEVEL == MAX_LEVEL) {
                        //Sort the level's data
                        post_M_N_Grams<TProbBucket, MAX_LEVEL>(m_N_gram_data);
                    } else {
                        if (CURR_LEVEL > M_GRAM_LEVEL_1) {
                            //Compute the M-gram level index
                            constexpr TModelLevel CURR_LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);

                            //Sort the level's data
                            post_M_N_Grams<TProbBackOffBucket, CURR_LEVEL>(m_M_gram_data[CURR_LEVEL_IDX]);
                        }
                    }
                };

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success.
                 * @see GenericTrieBase
                 */
                inline void get_unigram_payload(typename BASE::T_Query_Exec_Data & query) const {
                    //Get the uni-gram word index
                    const TModelLevel & word_idx = query.m_begin_word_idx;
                    //This is at least a uni-gram we have, therefore first process the it in a special way
                    const TShortId word_id = query.m_gram[word_idx];

                    //Store the uni-gram payload pointer and add the probability to the total conditional probability 
                    query.m_payloads[word_idx][word_idx] = &m_1_gram_data[word_id];
                    LOG_DEBUG << "Getting the uni-gram payload for word id " << SSTR(word_id) << ": " << (string) m_1_gram_data[word_id] << END_LOG;
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for 1<m<n
                 * @see GenericTrieBase
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_m_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    const TModelLevel curr_level = (query.m_end_word_idx - query.m_begin_word_idx) + 1;
                    const TModelLevel layer_idx = curr_level - BASE::MGRAM_IDX_OFFSET;
                    LOG_DEBUG << "Searching in " << SSTR(curr_level) << "-grams, array index: " << layer_idx << END_LOG;

                    //Call the templated part via function pointer
                    status = get_payload<TProbBackOffBucket>(m_bucket_dividers, m_M_gram_data[layer_idx], query);
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==n
                 * @see GenericTrieBase
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_n_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    LOG_DEBUG << "Searching in " << SSTR(MAX_LEVEL) << "-grams" << END_LOG;

                    //Call the templated part via function pointer
                    status = get_payload<TProbBucket>(m_bucket_dividers, m_N_gram_data, query);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~G2DMapTrie();

            private:

                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //These are arrays of buckets for M-Gram levels with 1 < M < N
                typedef DynamicStackArray<__G2DMapTrie::T_M_Gram_PB_Entry, uint8_t, 0, &__G2DMapTrie::destroy_Comp_M_Gram_Id<__G2DMapTrie::T_M_Gram_PB_Entry> > TProbBackOffBucket;
                TProbBackOffBucket * m_M_gram_data[BASE::NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef DynamicStackArray<__G2DMapTrie::T_M_Gram_Prob_Entry, uint8_t, 0, &__G2DMapTrie::destroy_Comp_M_Gram_Id<__G2DMapTrie::T_M_Gram_Prob_Entry> > TProbBucket;
                TProbBucket * m_N_gram_data;

                //Stores the number of gram ids/buckets per level
                uint32_t m_num_buckets[MAX_LEVEL];

                //Stores the bucket dividers per level
                uint32_t m_bucket_dividers[MAX_LEVEL];

                /**
                 * Allows to get the bucket index for the given M-gram hash
                 * @param curr_level the m-gram level we need the bucked id for
                 * @param gram_hash the M-gram hash to compute the bucked index for
                 * @param m_bucket_divider the number of buckers
                 * @param return the resulting bucket index
                 */
                static inline uint32_t get_bucket_id(const uint64_t gram_hash, const uint32_t m_bucket_divider) {
                    LOG_DEBUG1 << "The m-gram hash is: " << gram_hash << END_LOG;
                    uint32_t bucket_idx = gram_hash & m_bucket_divider;
                    LOG_DEBUG1 << "The m-gram bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    ASSERT_SANITY_THROW((bucket_idx > m_bucket_divider),
                            string("The m-gram has a bad bucket index: ") + std::to_string(bucket_idx) +
                            string(", must be within [0, ") + std::to_string(m_bucket_divider) + "]");

                    return bucket_idx;
                }

                //Declare the binary search function inlining the call to compare function.
                DECLARE_STATIC_BSEARCH_ID_FIELD_COMPARE_FUNC(
                        TM_Gram_Id::compare(len_bytes, id_ptr, array[mid_pos].id),
                        const uint8_t len_bytes, T_Gram_Id_Data_Ptr const & id_ptr);

                /**
                 * Gets the probability for the given level M-gram, searches on specific level
                 * @param BUCKET_TYPE the level bucket type
                 * @param query the query M-gram state 
                 * @param buckets the array of buckets to work with
                 * @param payload_ptr_ptr [out] the pointer to the payload pointer
                 * @return the resulting status of the operation
                 */
                template<typename BUCKET_TYPE>
                static inline MGramStatusEnum get_payload(const uint32_t m_bucket_divider[MAX_LEVEL], const BUCKET_TYPE * buckets,
                        typename BASE::T_Query_Exec_Data & query) {
                    //Compute the current level of the sub-m-gram
                    const TModelLevel curr_level = (query.m_end_word_idx - query.m_begin_word_idx) + 1;

                    LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(curr_level) << "-gram ["
                            << query.m_begin_word_idx << "," << query.m_end_word_idx << "] of: " << (string) query.m_gram << END_LOG;

                    const uint64_t hash_value = query.m_gram.get_hash(query.m_begin_word_idx, query.m_end_word_idx);
                    const uint32_t bucket_idx = get_bucket_id(hash_value, m_bucket_divider[curr_level - 1]);
                    LOG_DEBUG << "The " << SSTR(curr_level) << "-gram hash bucket idx is: " << bucket_idx << END_LOG;

                    LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(curr_level) << "-gram ["
                            << SSTR(query.m_begin_word_idx) << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;
                    //Get the bucket to look into
                    const BUCKET_TYPE & ref = buckets[bucket_idx];

                    if (ref.size() > 0) {
                        LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;

                        //Obtain the m-gram id and its length
                        uint8_t len_bytes;
                        T_Gram_Id_Data_Ptr id_ptr = query.m_gram.get_m_gram_id_ref(query.m_begin_word_idx, curr_level, len_bytes);

                        //Unroll the search into several specific cases, note the order is trying
                        //to put the most relevant cases first, it gives micro performance boost.
                        switch (ref.size()) {
                            case 2: //If there is two elements, perform an  explicit check
                            {
                                const typename BUCKET_TYPE::TElemType & elem2 = ref.data()[1];
                                if (TM_Gram_Id::compare(len_bytes, id_ptr, elem2.id) == 0) {
                                    query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &elem2.payload;
                                    //We are now done, the payload is found, can return!
                                    return MGramStatusEnum::GOOD_PRESENT_MGS;
                                }
                                //Move on to case 1
                            }
                            case 1: //If there is one element, perform an  explicit check
                            {
                                const typename BUCKET_TYPE::TElemType & elem1 = ref.data()[0];
                                if (TM_Gram_Id::compare(len_bytes, id_ptr, elem1.id) == 0) {
                                    query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &elem1.payload;
                                    //We are now done, the payload is found, can return!
                                    return MGramStatusEnum::GOOD_PRESENT_MGS;
                                }
                                break;
                            }
                            default: //If there is more than 3 elements then perform a binary search on data
                            {
                                const typename BUCKET_TYPE::TElemType * found_elem_ptr;
                                if (my_bsearch_id< typename BUCKET_TYPE::TElemType >
                                        (ref.data(), 0, ref.size() - 1, found_elem_ptr, len_bytes, id_ptr)) {
                                    query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &found_elem_ptr->payload;
                                    return MGramStatusEnum::GOOD_PRESENT_MGS;
                                }
                                break;
                            }
                        }
                    }

                    //Could not retrieve the payload for the given sub-m-gram
                    LOG_DEBUG << "Unable to find the sub-m-gram [" << SSTR(query.m_begin_word_idx)
                            << ", " << SSTR(query.m_end_word_idx) << "] payload!" << END_LOG;
                    return MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                }

                /**
                 * Performs the post-processing actions on the buckets in the given M-gram level
                 * @param BUCKET_TYPE the sort of buckets we should work with
                 * @param CURR_LEVEL the M-gram level value M
                 * @param buckets the pointer to the array of buckets to process
                 */
                template<typename BUCKET_TYPE, TModelLevel CURR_LEVEL >
                void post_M_N_Grams(BUCKET_TYPE * buckets) {
                    //Iterate through all buckets and shrink/sort sub arrays
                    for (TShortId bucket_idx = 0; bucket_idx < m_num_buckets[CURR_LEVEL - 1]; ++bucket_idx) {
                        //First get the sub-array reference. 
                        BUCKET_TYPE & ref = buckets[bucket_idx];

                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Reduce capacity if there is unused memory
                        ref.shrink();
                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;

                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;

                        //Get one of the id length in bytes
                        const uint8_t ID_TYPE_LEN_BYTES = TM_Gram_Id::ID_TYPE_LEN_BYTES[CURR_LEVEL];

                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort([&] (const typename BUCKET_TYPE::TElemType & first, const typename BUCKET_TYPE::TElemType & second) -> bool {
                            LOG_DEBUG1 << "Comparing " << SSTR((void*) first.id) << " with " << SSTR((void*) second.id) << END_LOG;
                            //Update the progress bar status
                            Logger::update_progress_bar();
                                    //Return the result
                            return TM_Gram_Id::template is_less_m_grams_id<CURR_LEVEL>(ID_TYPE_LEN_BYTES, first.id, second.id);
                        });
                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;
                    }
                }
            };

            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TG2DMapTrieBasic;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TG2DMapTrieCount;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TG2DMapTrieOptBasic;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TG2DMapTrieOptCount;
            typedef G2DMapTrie<M_GRAM_LEVEL_MAX, HashingWordIndex > TG2DMapTrieHashing;
        }
    }
}


#endif	/* G2DHASHMAPTRIE_HPP */

