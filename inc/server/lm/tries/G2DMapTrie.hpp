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

#include "server/lm/TrieConstants.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/AWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"
#include "server/lm/mgrams/ModelMGram.hpp"
#include "server/lm/mgrams/ByteMGramId.hpp"

#include "common/utils/file/text_piece_reader.hpp"

#include "common/utils/containers/array_utils.hpp"
#include "common/utils/containers/fixed_size_hashmap.hpp"

#include "GenericTrieBase.hpp"
#include "W2CArrayTrie.hpp"

using namespace std;

using namespace uva::utils::containers;
using namespace uva::utils::logging;
using namespace uva::utils::file;
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
                template<typename TPayloadType, typename TWordIdType, TModelLevel MAX_LEVEL>
                struct S_M_GramData {
                    //The m-gram id type
                    typedef Byte_M_Gram_Id<TWordIdType, MAX_LEVEL> TM_Gram_Id;
                    //The self typedef
                    typedef S_M_GramData<TPayloadType, TWordIdType, MAX_LEVEL> SELF;

                    //The m-gram id pointer
                    TM_Gram_Id_Value_Ptr m_id;
                    //The m-gram payload
                    TPayloadType m_payload;

                    /**
                     * The basic constructor
                     */
                    S_M_GramData() : m_id(NULL) {
                    }

                    /**
                     * The comparison operator, allows to  compare two m-gram ids
                     * @param id the m-gram id to compare with
                     * @return true if the ids are equal, otherwise false
                     */
                    inline bool operator==(const T_Gram_Id_Key & key) const {
                        return (TM_Gram_Id::compare(key.m_len_bytes, key.m_id, m_id) == 0);
                    }

                    /**
                     * Allows to clear the data allocated for the given element
                     * @param elem the element to clear
                     */
                    static inline void clear(SELF & elem) {
                        if (elem.m_id != NULL) {
                            m_gram_id::destroy(elem.m_id);
                        }
                    }
                };

#pragma pack(pop) //back to whatever the previous packing mode was 
            }

            /**
             * This is a Gram to Data trie that is implemented as a HashMap.
             * @param MAX_LEVEL - the maximum level of the considered N-gram, i.e. the N value
             */
            template<TModelLevel MAX_LEVEL, typename WordIndexType>
            class G2DMapTrie : public GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
            public:
                typedef GenericTrieBase<G2DMapTrie<MAX_LEVEL, WordIndexType>, MAX_LEVEL, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;
                typedef typename WordIndexType::TWordIdType TWordIdType;
                typedef __G2DMapTrie::S_M_GramData<T_M_Gram_Payload, TWordIdType, MAX_LEVEL> T_M_Gram_PB_Entry;
                typedef __G2DMapTrie::S_M_GramData<TLogProbBackOff, TWordIdType, MAX_LEVEL> T_M_Gram_Prob_Entry;

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

                        if (CURR_LEVEL == MAX_LEVEL) {
                            //Create a new M-Gram data entry
                            T_M_Gram_Prob_Entry & data = m_n_gram_data->add_new_element(gram.get_hash());
                            //Create the N-gram id from the word ids
                            gram.create_m_gram_id(gram.get_begin_word_idx(), CURR_LEVEL, data.m_id);
                            //Set the probability data
                            data.m_payload = gram.m_payload.m_prob;
                        } else {
                            //Compute the M-gram level index
                            constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                            //Create a new M-Gram data entry
                            T_M_Gram_PB_Entry & data = m_m_gram_data[LEVEL_IDX]->add_new_element(gram.get_hash());
                            //Create the M-gram id from the word ids.
                            gram.create_m_gram_id(gram.get_begin_word_idx(), CURR_LEVEL, data.m_id);
                            //Set the probability and back-off data
                            data.m_payload = gram.m_payload;
                        }
                    }
                }

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
                    //Get the current level for logging
                    const TModelLevel & curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];

                    //Get the current level of the sub-m-gram
                    const TModelLevel & layer_idx = CURR_LEVEL_MIN_2_MAP[query.m_begin_word_idx][query.m_end_word_idx];

                    LOG_DEBUG << "Searching in " << SSTR(curr_level) << "-grams, array index: " << layer_idx << END_LOG;

                    //Call the templated part via function pointer
                    status = get_payload<TProbBackMap>(m_m_gram_data[layer_idx], query);
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
                    status = get_payload<TProbMap>(m_n_gram_data, query);
                }

                /**
                 * The basic class destructor
                 */
                virtual ~G2DMapTrie();

            private:

                //Stores the 1-gram data
                T_M_Gram_Payload * m_1_gram_data;

                //This is an array of hash maps for M-Gram levels with 1 < M < N
                typedef FixedSizeHashMap<T_M_Gram_PB_Entry, T_Gram_Id_Key> TProbBackMap;
                TProbBackMap * m_m_gram_data[BASE::NUM_M_GRAM_LEVELS];

                //This is hash map pointer for the N-Gram level
                typedef FixedSizeHashMap<T_M_Gram_Prob_Entry, T_Gram_Id_Key> TProbMap;
                TProbMap * m_n_gram_data;

                /**
                 * Gets the probability for the given level M-gram, searches on specific level
                 * @param map the map storing the elements
                 * @param query the query object
                 * @return the resulting status of the operation
                 */
                template<typename STORAGE_MAP>
                static inline MGramStatusEnum get_payload(const STORAGE_MAP * map,
                        typename BASE::T_Query_Exec_Data & query) {
                    //Get the current level for logging
                    const TModelLevel & curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];

                    LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(curr_level) << "-gram ["
                            << query.m_begin_word_idx << "," << query.m_end_word_idx << "] of: " << (string) query.m_gram << END_LOG;

                    //Obtain the m-gram key
                    T_Gram_Id_Key key;
                    key.m_id = query.m_gram.get_m_gram_id_ref(query.m_begin_word_idx, curr_level, key.m_len_bytes);

                    //Get the hash value
                    const uint64_t hash_value = query.m_gram.get_hash(query.m_begin_word_idx, query.m_end_word_idx);

                    LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(curr_level) << "-gram ["
                            << SSTR(query.m_begin_word_idx) << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;

                    //Get the element from the map
                    const typename STORAGE_MAP::TElemType * elem = map->get_element(hash_value, key);
                    if (elem != NULL) {
                        //We are now done, the payload is found, can return!
                        query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &elem->m_payload;
                        return MGramStatusEnum::GOOD_PRESENT_MGS;
                    } else {
                        //Could not retrieve the payload for the given sub-m-gram
                        LOG_DEBUG << "Unable to find the sub-m-gram [" << SSTR(query.m_begin_word_idx)
                                << ", " << SSTR(query.m_end_word_idx) << "] payload!" << END_LOG;
                        return MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
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

