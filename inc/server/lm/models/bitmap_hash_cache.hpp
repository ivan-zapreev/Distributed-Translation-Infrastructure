/* 
 * File:   BitmapHashCache.hpp
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
 * Created on September 15, 2015, 8:37 AM
 */

#ifndef BITMAPHASHCACHE_HPP
#define BITMAPHASHCACHE_HPP

#include <cstdint>      //  std::uint8_t std::uint32_t 
#include <bitset>       //  std::bitset

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/mgrams/ModelMGram.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/math_utils.hpp"

using namespace std;

using namespace uva::utils::math;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::translation::server::lm::m_grams;
using namespace uva::smt::translation::server::lm::identifiers;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace caching {

                        /**
                         * This class is to be used for caching the present of M-grams in the trie.
                         * The way it is done is using a bitset. A bitset indicates which hashes
                         * are present and therefore which M-grams have a chance to be found in the
                         * Trie. This class can give potential speed improvement for the Tries which
                         * are context/layer based and use search algorithms to go through levels.
                         */
                        class BitmapHashCache {
                        public:

                            /**
                             * The basic constructor, does not do much - only default initialization
                             */
                            BitmapHashCache() : m_num_buckets(0), m_data_ptr(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            virtual ~BitmapHashCache() {
                                if (m_data_ptr != NULL) {
                                    delete[] m_data_ptr;
                                }
                            }

                            /**
                             * Allowo to pre-allocate memory for the bitset
                             * @param num_elems
                             * @return 
                             */
                            inline void pre_allocate(const size_t num_elems, const uint8_t buckets_factor) {
                                if (DO_SANITY_CHECKS && (m_data_ptr != NULL)) {
                                    throw Exception("The bitset is already pre-allocated!");
                                }

                                if (num_elems != 0) {
                                    //Compute the number of buckets as a power of two, so that we do not need to use %
                                    m_num_buckets = const_expr::power(2, const_expr::ceil(const_expr::log2(buckets_factor * num_elems)));
                                    m_buckets_capacity = m_num_buckets - 1;
                                    size_t num_bytes = NUM_BYTES_4_BITS(m_num_buckets);

                                    LOG_DEBUG << "num_elems: " << num_elems
                                            << " m_num_buckets: " << m_num_buckets
                                            << " bytes: " << num_bytes << END_LOG;

                                    m_data_ptr = new uint8_t[num_bytes];
                                    fill_n(m_data_ptr, num_bytes, 0u);
                                } else {
                                    throw Exception("Trying to pre-allocate 0 elements for a BitmaphashCache!");
                                }
                            }

                            /**
                             * Allows to add the M-gram to the cache
                             * @param gram the M-gram to cache
                             */
                            template<typename WordIndexType>
                            inline void cache_m_gram_hash(const T_Model_M_Gram<WordIndexType> gram) {
                                LOG_DEBUG2 << "Adding M-gram: " << (string) gram << END_LOG;

                                //Get the bit position
                                uint32_t byte_idx = 0;
                                uint32_t bit_offset_idx = 0;
                                get_bit_pos<WordIndexType>(gram, byte_idx, bit_offset_idx);

                                LOG_DEBUG2 << "Adding: " << bitset<NUM_BITS_IN_UINT_8>(ON_BIT_ARRAY[bit_offset_idx])
                                        << ", to: " << bitset<NUM_BITS_IN_UINT_8>(m_data_ptr[byte_idx]) << END_LOG;

                                //Set the bit on
                                m_data_ptr[byte_idx] |= ON_BIT_ARRAY[bit_offset_idx];
                            }

                            /**
                             * Allows to check if the given sub-m-gram, defined by the begin_word_idx
                             * and end_word_idx parameters, is potentially present in the trie.
                             * @param key the m-gram key
                             * @return true if the sub-m-gram is potentially present, otherwise false
                             */
                            inline bool is_hash_cached(uint_fast64_t key) const {

                                //Get the M-gram hash positions
                                uint32_t byte_idx = 0;
                                uint32_t bit_offset_idx = 0;
                                get_bit_pos(key, byte_idx, bit_offset_idx);

                                LOG_DEBUG2 << "Returning: " << bitset<NUM_BITS_IN_UINT_8>(m_data_ptr[byte_idx]) << " & "
                                        << bitset<NUM_BITS_IN_UINT_8>(ON_BIT_ARRAY[bit_offset_idx]) << END_LOG;

                                //Return the bit on
                                return (m_data_ptr[byte_idx] & ON_BIT_ARRAY[bit_offset_idx]);
                            }

                        private:
                            //Stores the number of elements this bitset was pre-allocated for
                            size_t m_num_buckets;
                            //Stores the buckets capacity that should be the number of buckets minus one
                            size_t m_buckets_capacity;
                            //Stores the data allocated for the bitset
                            uint8_t * m_data_ptr;

                            /**
                             * Allows to get the bit position for the M-gram
                             * @param key the M-gram key
                             * @param byte_idx [out] the M-gram byte index
                             * @param bit_offset_idx [out] the M-gram relative bit index
                             */
                            inline void get_bit_pos(uint_fast64_t & key, uint32_t & byte_idx, uint32_t & bit_offset_idx) const {
                                LOG_DEBUG2 << "The key value is: " << key
                                        << ", the number of elements is: " << m_num_buckets << END_LOG;

                                //Convert it to the number of elements, use m_buckets_capacity = m_num_buckets - 1;
                                //where m_num_buckets is the power of two, do not do extra shuffling of elements
                                //this does not really add any performance speed up to the models.
                                const uint32_t global_bit_idx = key & m_buckets_capacity;

                                //Convert the global bit index into the byte index and bit offset
                                byte_idx = BYTE_IDX(global_bit_idx);
                                bit_offset_idx = REMAINING_BIT_IDX(global_bit_idx);

                                LOG_DEBUG1 << "The M-gram hash: " << key << ", byte_idx: " << byte_idx
                                        << ", bit_offset_idx: " << bit_offset_idx << END_LOG;
                            }

                            /**
                             * Allows to get the bit position for the M-gram
                             * @param gram the M-gram to get position for
                             * @param byte_idx [out] the M-gram byte index
                             * @param bit_offset_idx [out] the M-gram relative bit index
                             */
                            template<typename WordIndexType>
                            inline void get_bit_pos(const T_Model_M_Gram<WordIndexType> &gram, uint32_t & byte_idx, uint32_t & bit_offset_idx) const {
                                uint_fast64_t key = gram.get_hash();

                                LOG_DEBUG2 << "The M-gram: " << (string) gram << " hash: " << key << END_LOG;

                                return get_bit_pos(key, byte_idx, bit_offset_idx);
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* BITMAPHASHCACHE_HPP */

