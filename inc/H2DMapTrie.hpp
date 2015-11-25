/* 
 * File:   H2DMapTrie.hpp
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
 * Created on November 23, 2015, 12:24 AM
 */

#ifndef H2DHASHMAPTRIE_HPP
#define	H2DHASHMAPTRIE_HPP

#include <string>       // std::string
#include <functional>   // std::function

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

            namespace __H2DMapTrie {

#pragma pack(push, 1) // exact fit - no padding

                /**
                 * This template structure is used for storing trie hash map elements
                 * Each element contains and id of the m-gram and its payload -
                 * the probability/back-off data, the latter is the template parameter
                 * @param id stores the M-gram id
                 * @param payload stores the payload which is either probability or probability with back-off
                 */
                template<typename PAYLOAD_TYPE>
                struct S_M_GramData {
                    uint64_t id; //8 byte
                    PAYLOAD_TYPE payload; //xxx byte 8 byte for T_M_Gram_Payload 4 byte for TLogProbBackOff

                    //Stores the memory increase strategy object
                    const static MemIncreaseStrategy m_mem_strat;
                    typedef uint64_t TIdType;
                };
#pragma pack(pop) //back to whatever the previous packing mode was 

                typedef S_M_GramData<T_M_Gram_Payload> T_M_Gram_PB_Entry;
                typedef S_M_GramData<TLogProbBackOff> T_M_Gram_Prob_Entry;

                template<typename PAYLOAD_TYPE>
                const MemIncreaseStrategy S_M_GramData<PAYLOAD_TYPE>::m_mem_strat =
                get_mem_incr_strat(__H2DMapTrie::MEM_INC_TYPE,
                        __H2DMapTrie::MIN_MEM_INC_NUM, __H2DMapTrie::MEM_INC_FACTOR);
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param MAX_LEVEL - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class H2DMapTrie : public GenericTrieBase<H2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __H2DMapTrie::DO_BITMAP_HASH_CACHE> {
            public:
                typedef GenericTrieBase<H2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __H2DMapTrie::DO_BITMAP_HASH_CACHE> BASE;

                //The typedef for the retrieving function
                typedef function<uint32_t(const H2DMapTrie&, const uint64_t gram_hash) > TGetBucketIdFunct;

                /**
                 * The basic constructor
                 * @param _wordIndex the word index to be used
                 */
                explicit H2DMapTrie(WordIndexType & word_index);

                /**
                 * Allows to log the information about the instantiated trie type
                 */
                inline void log_trie_type_usage_info() const {
                    LOG_USAGE << "Using the <" << __FILE__ << "> model." << END_LOG;
                    LOG_INFO << "Using the #buckets divider: "
                            << SSTR(__H2DMapTrie::WORDS_PER_BUCKET_FACTOR) << END_LOG;
                    LOG_INFO << "Using  and the " << __H2DMapTrie::T_M_Gram_PB_Entry::m_mem_strat.get_strategy_info()
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
                    //Register the m-gram in the hash cache
                    this->template register_m_gram_cache<CURR_LEVEL>(gram);

                    //Compute the M-gram level index, here we store m-grams for 1 <=m < n in one structure
                    constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - LEVEL_IDX_OFFSET);

                    //Get the bucket index
                    const uint64_t hash_value = gram.get_hash();
                    LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << " hash value: " << hash_value << END_LOG;
                    TShortId bucket_idx = get_bucket_id(hash_value, m_num_buckets[LEVEL_IDX]);

                    if (CURR_LEVEL == MAX_LEVEL) {
                        //Create a new M-Gram data entry
                        __H2DMapTrie::T_M_Gram_Prob_Entry & data = m_n_gram_data[bucket_idx].allocate();
                        //The n-gram id is equal to its hash value
                        data.id = hash_value;
                        //Set the probability data
                        data.payload = gram.m_payload.m_prob;
                    } else {
                        //Check if this is an <unk> unigram, in this case we store the payload elsewhere
                        if ((CURR_LEVEL == M_GRAM_LEVEL_1) && gram.is_unk_unigram()) {
                            //Store the uni-gram payload - overwrite the default values.
                            m_unk_word_payload = gram.m_payload;
                        } else {
                            //Create a new M-Gram data entry
                            __H2DMapTrie::T_M_Gram_PB_Entry & data = m_m_gram_data[LEVEL_IDX][bucket_idx].allocate();
                            //The m-gram id is equal to its hash value
                            data.id = hash_value;
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
                    //We need post m-gram actions for all m-gram level values
                    return true;
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
                        post_m_n_grams<TProbBucket, MAX_LEVEL>(m_n_gram_data);
                    } else {
                        //Compute the M-gram level index
                        constexpr TModelLevel CURR_LEVEL_IDX = (CURR_LEVEL - LEVEL_IDX_OFFSET);

                        //Sort the level's data
                        post_m_n_grams<TProbBackOffBucket, CURR_LEVEL>(m_m_gram_data[CURR_LEVEL_IDX]);
                    }
                };

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for m==1.
                 * The retrieval of a uni-gram data is always a success.
                 * @see GenericTrieBase
                 */
                inline void get_unigram_payload(typename BASE::T_Query_Exec_Data & query) const {
                    LOG_DEBUG << "Searching in uni-grams, array index: 0" << END_LOG;

                    //By default set the unknown word value
                    query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &m_unk_word_payload;

                    //Call the templated part via function pointer
                    (void) get_payload<TProbBackOffBucket>(m_num_buckets, m_m_gram_data[0], query);
                }

                /**
                 * Allows to attempt the sub-m-gram payload retrieval for 1<m<n
                 * @see GenericTrieBase
                 * @param query the query containing the actual query data
                 * @param status the resulting status of the operation
                 */
                inline void get_m_gram_payload(typename BASE::T_Query_Exec_Data & query, MGramStatusEnum & status) const {
                    const TModelLevel curr_level = (query.m_end_word_idx - query.m_begin_word_idx) + 1;
                    const TModelLevel layer_idx = curr_level - LEVEL_IDX_OFFSET;
                    LOG_DEBUG << "Searching in " << SSTR(curr_level) << "-grams, array index: " << layer_idx << END_LOG;

                    //Call the templated part via function pointer
                    status = get_payload<TProbBackOffBucket>(m_num_buckets, m_m_gram_data[layer_idx], query);
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
                    status = get_payload<TProbBucket>(m_num_buckets, m_n_gram_data, query);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~H2DMapTrie();

            private:

                //The offset, relative to the M-gram level M for the m-gram mapping array index
                const static TModelLevel LEVEL_IDX_OFFSET = 1;

                //Will store the the number of M levels such that 1 <= M < N.
                const static TModelLevel NUM_M_GRAM_LEVELS = MAX_LEVEL - LEVEL_IDX_OFFSET;

                //Stores the unknown word payload data
                T_M_Gram_Payload m_unk_word_payload;

                //typedef the bucket capacity type, for convenience.
                typedef uint16_t TBucketCapacityType;

                //These are arrays of buckets for M-Gram levels with 1 <= M < N
                typedef DynamicStackArray<__H2DMapTrie::T_M_Gram_PB_Entry, TBucketCapacityType > TProbBackOffBucket;
                TProbBackOffBucket * m_m_gram_data[NUM_M_GRAM_LEVELS];

                //This is an array of buckets for the N-Gram level
                typedef DynamicStackArray<__H2DMapTrie::T_M_Gram_Prob_Entry, TBucketCapacityType > TProbBucket;
                TProbBucket * m_n_gram_data;

                //Stores the number of m-gram ids/buckets per level
                TShortId m_num_buckets[MAX_LEVEL];

                /**
                 * Allows to get the bucket index for the given M-gram hash
                 * @param curr_level the m-gram level we need the bucked id for
                 * @param gram_hash the M-gram hash to compute the bucked index for
                 * @param num_buckets the number of buckers
                 * @param return the resulting bucket index
                 */
                static inline TShortId get_bucket_id(const uint64_t gram_hash, const TShortId num_buckets) {
                    LOG_DEBUG1 << "The m-gram hash is: " << gram_hash << END_LOG;
                    TShortId bucket_idx = gram_hash % num_buckets;
                    LOG_DEBUG1 << "The m-gram bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    ASSERT_SANITY_THROW((bucket_idx >= num_buckets), string("The m-gram has a bad bucket index: ") +
                            std::to_string(bucket_idx) + ", must be within [0, " + std::to_string(num_buckets));

                    return bucket_idx;
                }

                /**
                 * Gets the probability for the given level M-gram, searches on specific level
                 * @param BUCKET_TYPE the level bucket type
                 * @param query the query M-gram state 
                 * @param buckets the array of buckets to work with
                 * @param payload_ptr_ptr [out] the pointer to the payload pointer
                 * @param status [out] the resulting status of the operation
                 */
                template<typename BUCKET_TYPE>
                static inline MGramStatusEnum get_payload(const TShortId num_buckets[MAX_LEVEL], const BUCKET_TYPE * buckets,
                        typename BASE::T_Query_Exec_Data & query) {
                    //Compute the current level of the sub-m-gram
                    const TModelLevel curr_level = (query.m_end_word_idx - query.m_begin_word_idx) + 1;

                    LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(curr_level) << "-gram ["
                            << query.m_begin_word_idx << "," << query.m_end_word_idx << "] of: " << (string) query.m_gram << END_LOG;

                    const uint64_t hash_value = query.m_gram.template get_hash(query.m_begin_word_idx, query.m_end_word_idx);
                    const uint32_t bucket_idx = get_bucket_id(hash_value, num_buckets[curr_level - 1]);
                    LOG_DEBUG << "The " << SSTR(curr_level) << "-gram hash bucket idx is: " << bucket_idx << END_LOG;

                    LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(curr_level) << "-gram ["
                            << SSTR(query.m_begin_word_idx) << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;
                    //Get the bucket to look into
                    const BUCKET_TYPE & ref = buckets[bucket_idx];

                    //Check that the bucket with the given index is not empty
                    if (ref.has_data()) {
                        LOG_DEBUG << "The bucket contains " << ref.size() << " elements!" << END_LOG;

                        //Search for the query id in the bucket, the query id is its hash value.
                        const typename BUCKET_TYPE::TElemType * elem_ptr;
                        if (my_bsearch_id< typename BUCKET_TYPE::TElemType >
                                (ref.data(), 0, ref.size() - 1, hash_value, elem_ptr)) {
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &elem_ptr->payload;
                            //We are now done, the payload is found, can return!
                            return MGramStatusEnum::GOOD_PRESENT_MGS;
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
                void post_m_n_grams(BUCKET_TYPE * buckets) {
                    //Iterate through all buckets and shrink/sort sub arrays
                    for (TShortId bucket_idx = 0; bucket_idx < m_num_buckets[CURR_LEVEL - 1]; ++bucket_idx) {
                        //First get the sub-array reference. 
                        BUCKET_TYPE & ref = buckets[bucket_idx];

                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Reduce capacity if there is unused memory
                        ref.shrink();
                        LOG_DEBUG1 << "Shrinking the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;

                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " ..." << END_LOG;
                        //Order the N-gram array as it is unordered and we will binary search it later!
                        ref.sort([&] (const typename BUCKET_TYPE::TElemType & first, const typename BUCKET_TYPE::TElemType & second) -> bool {
                            LOG_DEBUG1 << "Comparing " << first.id << " with " << second.id << END_LOG;
                            //Update the progress bar status
                            Logger::update_progress_bar();
                                    //Return the result
                            return (first.id < second.id);
                        });
                        LOG_DEBUG1 << "Sorting the " << SSTR(CURR_LEVEL) << "-gram level bucket idx: " << SSTR(bucket_idx) << " is done" << END_LOG;
                    }
                }
            };

            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, BasicWordIndex > TH2DMapTrieBasic;
            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, CountingWordIndex > TH2DMapTrieCount;
            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, TOptBasicWordIndex > TH2DMapTrieOptBasic;
            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, TOptCountWordIndex > TH2DMapTrieOptCount;
            typedef H2DMapTrie<M_GRAM_LEVEL_MAX, HashingWordIndex > TH2DMapTrieHashing;
        }
    }
}


#endif	/* H2DHASHMAPTRIE_HPP */

