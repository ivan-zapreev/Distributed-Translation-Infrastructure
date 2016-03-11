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
#define G2DHASHMAPTRIE_HPP

#include <string>       // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/dictionaries/aword_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"
#include "server/lm/mgrams/model_m_gram.hpp"
#include "server/lm/mgrams/m_gram_id.hpp"

#include "common/utils/file/text_piece_reader.hpp"

#include "common/utils/containers/array_utils.hpp"
#include "common/utils/containers/fixed_size_hashmap.hpp"

#include "generic_trie_base.hpp"
#include "w2c_array_trie.hpp"

using namespace std;

using namespace uva::utils::containers;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::m_grams;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    namespace __G2DMapTrie {

#pragma pack(push, 1) // exact fit - no padding

                        /**
                         * This template structure is used for storing trie hash map elements
                         * Each element contains and id of the m-gram and its payload -
                         * the probability/back-off data, the latter is the template parameter
                         * 
                         * NOTE: In order to save space and increase the speed we could store key to
                         *       be the hash value of the m-gram, but then we will get the h2dm trie.
                         * 
                         * @param id stores the M-gram id
                         * @param payload stores the payload which is either probability or probability with back-off
                         */
                        template<typename TPayloadType, typename TWordIdType>
                        struct S_M_GramData {
                            //The m-gram id type
                            typedef Byte_M_Gram_Id<TWordIdType> TM_Gram_Id;
                            //The self typedef
                            typedef S_M_GramData<TPayloadType, TWordIdType> SELF;

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
                             * The basic destructor
                             */
                            ~S_M_GramData() {
                                if (m_id != NULL) {
                                    m_gram_id::destroy(m_id);
                                }
                            }

                            /**
                             * The comparison operator, allows to  compare two m-gram ids
                             * @param id the m-gram id to compare with
                             * @return true if the ids are equal, otherwise false
                             */
                            inline bool operator==(const T_Gram_Id_Key & key) const {
                                return (TM_Gram_Id::compare(key.m_len_bytes, key.m_id, m_id) == 0);
                            }
                        };

#pragma pack(pop) //back to whatever the previous packing mode was 
                    }

                    /**
                     * This is a Gram to Data trie that is implemented as a HashMap.
                     * @param M_GRAM_LEVEL_MAX - the maximum level of the considered N-gram, i.e. the N value
                     */
                    template<typename WordIndexType>
                    class g2d_map_trie : public generic_trie_base<g2d_map_trie<WordIndexType>, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> {
                    public:
                        typedef generic_trie_base<g2d_map_trie<WordIndexType>, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR> BASE;
                        typedef __G2DMapTrie::S_M_GramData<m_gram_payload, word_uid> T_M_Gram_PB_Entry;
                        typedef __G2DMapTrie::S_M_GramData<prob_weight, word_uid> T_M_Gram_Prob_Entry;

                        /**
                         * The basic constructor
                         * @param _wordIndex the word index to be used
                         */
                        explicit g2d_map_trie(WordIndexType & word_index);

                        /**
                         * Allows to retrieve the unknown target word log probability penalty 
                         * @return the target source word log probability penalty
                         */
                        inline float get_unk_word_prob() const {
                            return m_unk_data->m_prob;
                        }

                        /**
                         * Allows to log the information about the instantiated trie type
                         */
                        inline void log_model_type_info() const {
                            LOG_USAGE << "Using the <" << __FILENAME__ << "> model." << END_LOG;
                            LOG_INFO << "The <" << __FILENAME__ << "> model's buckets factor: "
                                    << __G2DMapTrie::BUCKETS_FACTOR << END_LOG;
                        }

                        /**
                         * @see word_index_trie_base
                         */
                        void set_def_unk_word_prob(const prob_weight prob);

                        /**
                         * This method can be used to provide the N-gram count information
                         * That should allow for pre-allocation of the memory
                         * @see GenericTrieBase
                         */
                        virtual void pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]);

                        /**
                         * This method adds a M-Gram (word) to the trie where 1 < M < N
                         * @see GenericTrieBase
                         */
                        template<phrase_length CURR_LEVEL>
                        inline void add_m_gram(const model_m_gram & gram) {
                            if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                                //Get the word id of this unigram, so there is just one word in it and its the end one
                                const TShortId word_id = gram.get_last_word_id();
                                //Store the probability data in the one gram data storage, under its id
                                m_1_gram_data[word_id] = gram.m_payload;
                            } else {
                                //Register the m-gram in the hash cache
                                this->register_m_gram_cache(gram);

                                if (CURR_LEVEL == LM_M_GRAM_LEVEL_MAX) {
                                    //Create a new M-Gram data entry
                                    T_M_Gram_Prob_Entry & data = m_n_gram_data->add_new_element(gram.get_hash());
                                    //Create the N-gram id from the word ids
                                    gram.create_phrase_id(gram.get_first_word_idx(), CURR_LEVEL, data.m_id);
                                    //Set the probability data
                                    data.m_payload = gram.m_payload.m_prob;
                                } else {
                                    //Compute the M-gram level index
                                    constexpr phrase_length LEVEL_IDX = (CURR_LEVEL - BASE::MGRAM_IDX_OFFSET);
                                    //Create a new M-Gram data entry
                                    T_M_Gram_PB_Entry & data = m_m_gram_data[LEVEL_IDX]->add_new_element(gram.get_hash());
                                    //Create the M-gram id from the word ids.
                                    gram.create_phrase_id(gram.get_first_word_idx(), CURR_LEVEL, data.m_id);
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
                        inline void get_unigram_payload(m_gram_query & query) const {
                            //Get the uni-gram word id
                            const word_uid word_id = query.get_curr_uni_gram_word_id();

                            //Store the uni-gram payload pointer and add the probability to the total conditional probability 
                            query.set_curr_payload(&m_1_gram_data[word_id]);

                            LOG_DEBUG << "The uni-gram word id " << SSTR(word_id) << " payload : "
                                    << m_1_gram_data[word_id] << END_LOG;
                        }

                        /**
                         * Allows to attempt the sub-m-gram payload retrieval for 1<m<n
                         * @see GenericTrieBase
                         * @param query the query containing the actual query data
                         * @param status the resulting status of the operation
                         */
                        inline void get_m_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            //Get the current level for logging
                            const phrase_length & curr_level = query.get_curr_level();

                            //Get the current level of the sub-m-gram
                            const phrase_length & layer_idx = query.get_curr_level_m2();

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
                        inline void get_n_gram_payload(m_gram_query & query, MGramStatusEnum & status) const {
                            LOG_DEBUG << "Searching in " << SSTR(LM_M_GRAM_LEVEL_MAX) << "-grams" << END_LOG;

                            //Call the templated part via function pointer
                            status = get_payload<TProbMap>(m_n_gram_data, query);
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~g2d_map_trie();

                    private:
                        //Stores the pointer to the UNK word payload
                        m_gram_payload * m_unk_data;

                        //Stores the 1-gram data
                        m_gram_payload * m_1_gram_data;

                        //This is an array of hash maps for M-Gram levels with 1 < M < N
                        typedef fixed_size_hashmap<T_M_Gram_PB_Entry, T_Gram_Id_Key> TProbBackMap;
                        TProbBackMap * m_m_gram_data[BASE::NUM_M_GRAM_LEVELS];

                        //This is hash map pointer for the N-Gram level
                        typedef fixed_size_hashmap<T_M_Gram_Prob_Entry, T_Gram_Id_Key> TProbMap;
                        TProbMap * m_n_gram_data;

                        /**
                         * Gets the probability for the given level M-gram, searches on specific level
                         * @param map the map storing the elements
                         * @param query the query object
                         * @return the resulting status of the operation
                         */
                        template<typename STORAGE_MAP>
                        static inline MGramStatusEnum get_payload(const STORAGE_MAP * map,
                                m_gram_query & query) {

                            LOG_DEBUG << "Getting the bucket id for: " << query << END_LOG;

                            //Obtain the m-gram key
                            T_Gram_Id_Key key;
                            key.m_id = query.get_curr_m_gram_id(key.m_len_bytes);

                            //Get the hash value
                            const uint64_t hash_value = query.get_curr_m_gram_hash();

                            LOG_DEBUG << "Retrieving payload for " << query << END_LOG;

                            //Get the element from the map
                            const typename STORAGE_MAP::TElemType * elem = map->get_element(hash_value, key);
                            if (elem != NULL) {
                                //We are now done, the payload is found, can return!
                                query.set_curr_payload(&elem->m_payload);
                                return MGramStatusEnum::GOOD_PRESENT_MGS;
                            } else {
                                //Could not retrieve the payload for the given sub-m-gram
                                LOG_DEBUG << "Unable to find the payload for sub-m-gram : " << query << END_LOG;
                                return MGramStatusEnum::BAD_NO_PAYLOAD_MGS;
                            }
                        }
                    };

                    typedef g2d_map_trie<basic_word_index > TG2DMapTrieBasic;
                    typedef g2d_map_trie<counting_word_index > TG2DMapTrieCount;
                    typedef g2d_map_trie<basic_optimizing_word_index > TG2DMapTrieOptBasic;
                    typedef g2d_map_trie<counting_optimizing_word_index > TG2DMapTrieOptCount;
                    typedef g2d_map_trie<hashing_word_index > TG2DMapTrieHashing;
                }
            }
        }
    }
}

#endif /* G2DHASHMAPTRIE_HPP */

