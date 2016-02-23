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
#define H2DHASHMAPTRIE_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/AWordIndex.hpp"
#include "server/lm/dictionaries/HashingWordIndex.hpp"
#include "server/lm/mgrams/model_m_gram.hpp"
#include "server/lm/mgrams/m_gram_id.hpp"

#include "common/utils/file/text_piece_reader.hpp"

#include "common/utils/containers/array_utils.hpp"
#include "common/utils/containers/fixed_size_hashmap.hpp"

#include "generic_trie_base.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::containers;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::m_grams;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    namespace __H2DMapTrie {

#pragma pack(push, 1) // exact fit - no padding

                        /**
                         * This template structure is used for storing trie hash map elements
                         * Each element contains and id of the m-gram and its payload -
                         * the probability/back-off data, the latter is the template parameter
                         * @param id stores the M-gram id
                         * @param payload stores the payload which is either probability or probability with back-off
                         */
                        template<typename TPayloadType>
                        struct S_M_GramData {
                            //The m-gram id type
                            typedef uint64_t TM_Gram_Id;
                            //The self typedef
                            typedef S_M_GramData<TPayloadType> SELF;

                            //The field storing the m-gram id
                            TM_Gram_Id m_id; //8 byte

                            //The field storing the m-gram payload
                            TPayloadType m_payload; //xxx byte 8 byte for T_M_Gram_Payload 4 byte for TLogProbBackOff

                            /**
                             * The basic constructor
                             */
                            S_M_GramData() : m_id(0) {
                            }
                            
                            /**
                             * The basic destructor
                             */
                            ~S_M_GramData(){
                                //There is nothing to be cleared
                            }

                            /**
                             * The comparison operator, allows to  compare two m-gram ids
                             * @param id the m-gram id to compare with
                             * @return true if the ids are equal, otherwise false
                             */
                            inline bool operator==(const TM_Gram_Id & id) const {
                                return (m_id == id);
                            }
                        };
#pragma pack(pop) //back to whatever the previous packing mode was 
                    }

                    /**
                     * This is a Gram to Data trie that is implemented as a HashMap.
                     * @param M_GRAM_LEVEL_MAX - the maximum level of the considered N-gram, i.e. the N value
                     */
                    template<typename WordIndexType>
                    class H2DMapTrie : public GenericTrieBase<H2DMapTrie<WordIndexType>, WordIndexType, __H2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef GenericTrieBase<H2DMapTrie<WordIndexType>, WordIndexType, __H2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;
                        typedef typename WordIndexType::TWordIdType TWordIdType;
                        typedef __H2DMapTrie::S_M_GramData<m_gram_payload> T_M_Gram_PB_Entry;
                        typedef __H2DMapTrie::S_M_GramData<TLogProbBackOff> T_M_Gram_Prob_Entry;

                        /**
                         * The basic constructor
                         * @param _wordIndex the word index to be used
                         */
                        explicit H2DMapTrie(WordIndexType & word_index);

                        /**
                         * Allows to retrieve the unknown target word log probability penalty 
                         * @return the target source word log probability penalty
                         */
                        inline float get_unk_word_prob() const {
                            return m_unk_data.m_prob;
                        }

                        /**
                         * Allows to log the information about the instantiated trie type
                         */
                        inline void log_model_type_info() const {
                            LOG_USAGE << "Using the <" << __FILENAME__ << "> model." << END_LOG;
                            LOG_INFO << "Using the #buckets factor: "
                                    << SSTR(__H2DMapTrie::BUCKETS_FACTOR) << END_LOG;
                        }

                        /**
                         * This method can be used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @see GenericTrieBase
                         */
                        virtual void pre_allocate(const size_t counts[M_GRAM_LEVEL_MAX]);

                        /**
                         * This method adds a M-Gram (word) to the trie where 1 < M < N
                         * @see GenericTrieBase
                         */
                        template<TModelLevel CURR_LEVEL>
                        inline void add_m_gram(const model_m_gram<WordIndexType> & gram) {
                            //If not a uni-gram then register in the cache
                            if (CURR_LEVEL != M_GRAM_LEVEL_1) {
                                //Register the m-gram in the hash cache
                                this->register_m_gram_cache(gram);
                            }

                            //Compute the M-gram level index, here we store m-grams for 1 <=m < n in one structure
                            constexpr TModelLevel LEVEL_IDX = (CURR_LEVEL - LEVEL_IDX_OFFSET);

                            //Get the bucket index
                            const uint64_t hash_value = gram.get_hash();
                            LOG_DEBUG << "Getting the bucket id for the m-gram: " << (string) gram << " hash value: " << hash_value << END_LOG;

                            if (CURR_LEVEL == M_GRAM_LEVEL_MAX) {
                                //Create a new M-Gram data entry
                                T_M_Gram_Prob_Entry & data = m_n_gram_data->add_new_element(hash_value);
                                //The n-gram id is equal to its hash value
                                data.m_id = hash_value;
                                //Set the probability data
                                data.m_payload = gram.m_payload.m_prob;
                            } else {
                                //Check if this is an <unk> unigram, in this case we store the payload elsewhere
                                if ((CURR_LEVEL == M_GRAM_LEVEL_1) && gram.is_unk_unigram()) {
                                    //Store the uni-gram payload - overwrite the default values.
                                    m_unk_data = gram.m_payload;
                                } else {
                                    //Create a new M-Gram data entry
                                    T_M_Gram_PB_Entry & data = m_m_gram_data[LEVEL_IDX]->add_new_element(hash_value);
                                    //The m-gram id is equal to its hash value
                                    data.m_id = hash_value;
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
                        inline void get_unigram_payload(typename BASE::query_exec_data & query) const {
                            LOG_DEBUG << "Searching in uni-grams, array index: 0" << END_LOG;

                            //By default set the unknown word value
                            query.m_payloads[query.m_begin_word_idx][query.m_end_word_idx] = &m_unk_data;

                            //Call the templated part via function pointer
                            (void) get_payload<TProbBackMap>(m_m_gram_data[0], query);
                        }

                        /**
                         * Allows to attempt the sub-m-gram payload retrieval for 1<m<n
                         * @see GenericTrieBase
                         * @param query the query containing the actual query data
                         * @param status the resulting status of the operation
                         */
                        inline void get_m_gram_payload(typename BASE::query_exec_data & query, MGramStatusEnum & status) const {
                            //Get the current level for logging
                            const TModelLevel & curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];
                            //Get the current level of the sub-m-gram
                            const TModelLevel & layer_idx = CURR_LEVEL_MIN_1_MAP[query.m_begin_word_idx][query.m_end_word_idx];

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
                        inline void get_n_gram_payload(typename BASE::query_exec_data & query, MGramStatusEnum & status) const {
                            LOG_DEBUG << "Searching in " << SSTR(M_GRAM_LEVEL_MAX) << "-grams" << END_LOG;

                            //Call the templated part via function pointer
                            status = get_payload<TProbMap>(m_n_gram_data, query);
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~H2DMapTrie();

                    private:
                        //Stores the unknown word payload data
                        m_gram_payload m_unk_data;

                        //The offset, relative to the M-gram level M for the m-gram mapping array index
                        const static TModelLevel LEVEL_IDX_OFFSET = 1;

                        //Will store the the number of M levels such that 1 <= M < N.
                        const static TModelLevel NUM_M_GRAM_LEVELS = M_GRAM_LEVEL_MAX - LEVEL_IDX_OFFSET;

                        //typedef the bucket capacity type, for convenience.
                        typedef uint16_t TBucketCapacityType;

                        //This is an array of hash maps for M-Gram levels with 1 < M < N
                        typedef fixed_size_hashmap<T_M_Gram_PB_Entry, T_M_Gram_PB_Entry::TM_Gram_Id > TProbBackMap;
                        TProbBackMap * m_m_gram_data[NUM_M_GRAM_LEVELS];

                        //This is hash map pointer for the N-Gram level
                        typedef fixed_size_hashmap<T_M_Gram_Prob_Entry, T_M_Gram_Prob_Entry::TM_Gram_Id > TProbMap;
                        TProbMap * m_n_gram_data;

                        //Stores the number of m-gram ids/buckets per level
                        TShortId m_num_buckets[M_GRAM_LEVEL_MAX];

                        /**
                         * Gets the probability for the given level M-gram, searches on specific level
                         * @param STORAGE_MAP the level map type
                         * @param query the query M-gram state 
                         * @return the resulting status of the operation
                         */
                        template<typename STORAGE_MAP>
                        static inline MGramStatusEnum get_payload(const STORAGE_MAP * map,
                                typename BASE::query_exec_data & query) {
                            //Get the current level for logging
                            const TModelLevel & curr_level = CURR_LEVEL_MAP[query.m_begin_word_idx][query.m_end_word_idx];

                            LOG_DEBUG << "Getting the bucket id for the sub-" << SSTR(curr_level) << "-gram ["
                                    << query.m_begin_word_idx << "," << query.m_end_word_idx << "] of: " << (string) query.m_gram << END_LOG;

                            const uint64_t hash_value = query.m_gram.template get_hash(query.m_begin_word_idx, query.m_end_word_idx);

                            LOG_DEBUG << "Retrieving payload for a sub-" << SSTR(curr_level) << "-gram ["
                                    << SSTR(query.m_begin_word_idx) << ", " << SSTR(query.m_end_word_idx) << "]" << END_LOG;

                            //Get the element from the map, note that the key is the hash value
                            const typename STORAGE_MAP::TElemType * elem = map->get_element(hash_value, hash_value);
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

                    typedef H2DMapTrie<BasicWordIndex > TH2DMapTrieBasic;
                    typedef H2DMapTrie<CountingWordIndex > TH2DMapTrieCount;
                    typedef H2DMapTrie<TOptBasicWordIndex > TH2DMapTrieOptBasic;
                    typedef H2DMapTrie<TOptCountWordIndex > TH2DMapTrieOptCount;
                    typedef H2DMapTrie<HashingWordIndex > TH2DMapTrieHashing;
                }
            }
        }
    }
}

#endif /* H2DHASHMAPTRIE_HPP */

