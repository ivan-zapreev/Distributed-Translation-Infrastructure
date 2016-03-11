/* 
 * File:   G2DMapTrie.cpp
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
 * Created on September 8, 2015, 11:22 AM
 */

#include "server/lm/models/g2d_map_trie.hpp"

#include <inttypes.h>   // std::uint32_t
#include <algorithm>    // std::max

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"

using namespace uva::smt::bpbd::server::lm::dictionary;
using namespace uva::smt::bpbd::server::lm::__G2DMapTrie;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {

                    template<typename WordIndexType>
                    g2d_map_trie<WordIndexType>::g2d_map_trie(WordIndexType & word_index)
                    : generic_trie_base<g2d_map_trie<WordIndexType>, WordIndexType, __G2DMapTrie::BITMAP_HASH_CACHE_BUCKETS_FACTOR>(word_index),
                    m_unk_data(NULL), m_1_gram_data(NULL), m_n_gram_data(NULL) {
                        //Perform an error check! This container has bounds on the supported trie level
                        ASSERT_CONDITION_THROW((LM_M_GRAM_LEVEL_MAX > M_GRAM_LEVEL_6), string("The maximum supported trie level is") + std::to_string(M_GRAM_LEVEL_6));
                        ASSERT_CONDITION_THROW((!word_index.is_word_index_continuous()), "This trie can not be used with a discontinuous word index!");
                        ASSERT_CONDITION_THROW((sizeof (uint32_t) != sizeof (word_uid)), string("Only works with a 32 bit word_uid!"));

                        //Clear the M-Gram bucket arrays
                        memset(m_m_gram_data, 0, BASE::NUM_M_GRAM_LEVELS * sizeof (TProbBackMap*));

                        LOG_DEBUG << "sizeof(T_M_Gram_PB_Entry)= " << sizeof (T_M_Gram_PB_Entry) << END_LOG;
                        LOG_DEBUG << "sizeof(T_M_Gram_Prob_Entry)= " << sizeof (T_M_Gram_Prob_Entry) << END_LOG;
                        LOG_DEBUG << "sizeof(TProbBackMap)= " << sizeof (TProbBackMap) << END_LOG;
                        LOG_DEBUG << "sizeof(TProbMap)= " << sizeof (TProbMap) << END_LOG;
                    };

                    template<typename WordIndexType>
                    void g2d_map_trie<WordIndexType>::pre_allocate(const size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                        //Call the base-class
                        BASE::pre_allocate(counts);

                        //Pre-allocate the 1-Gram data
                        const size_t num_words = BASE::get_word_index().get_number_of_words(counts[0]);
                        m_1_gram_data = new m_gram_payload[num_words];
                        memset(m_1_gram_data, 0, num_words * sizeof (m_gram_payload));

                        //Initialize the m-gram maps
                        for (phrase_length idx = 1; idx <= BASE::NUM_M_GRAM_LEVELS; ++idx) {
                            m_m_gram_data[idx - 1] = new TProbBackMap(__G2DMapTrie::BUCKETS_FACTOR, counts[idx]);
                        }

                        //Initialize the n-gram's map
                        m_n_gram_data = new TProbMap(__G2DMapTrie::BUCKETS_FACTOR, counts[LM_M_GRAM_LEVEL_MAX - 1]);
                    };

                    template<typename WordIndexType>
                    void g2d_map_trie<WordIndexType>::set_def_unk_word_prob(const prob_weight prob) {
                        //Insert the unknown word data into the allocated array
                        m_unk_data = &m_1_gram_data[UNKNOWN_WORD_ID];
                        m_unk_data->m_prob = prob;
                        m_unk_data->m_back = 0.0;
                    }

                    template<typename WordIndexType>
                    g2d_map_trie<WordIndexType>::~g2d_map_trie() {
                        //Check that the one grams were allocated, if yes then the rest must have been either
                        if (m_1_gram_data != NULL) {
                            //De-allocate one grams
                            delete[] m_1_gram_data;
                            //De-allocate m-gram maps
                            for (phrase_length idx = 0; idx < BASE::NUM_M_GRAM_LEVELS; idx++) {
                                delete m_m_gram_data[idx];
                            }
                            //De-allocate n-grams map
                            delete m_n_gram_data;
                        }
                    };

                    INSTANTIATE_TRIE_TEMPLATE_TYPE(g2d_map_trie, basic_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(g2d_map_trie, counting_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(g2d_map_trie, hashing_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(g2d_map_trie, basic_optimizing_word_index);
                    INSTANTIATE_TRIE_TEMPLATE_TYPE(g2d_map_trie, counting_optimizing_word_index);
                }
            }
        }
    }
}