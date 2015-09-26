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
#define	BITMAPHASHCACHE_HPP

#include <cstdint>      //  std::uint8_t std::uint32_t 
#include <bitset>       //  std::bitset

#include "Globals.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include "MGrams.hpp"
#include "HashingUtils.hpp"
#include "MathUtils.hpp"
#include "MGramQuery.hpp"

using namespace std;

using namespace uva::utils::math;
using namespace uva::smt::logging;
using namespace uva::smt::hashing;
using namespace uva::smt::exceptions;
using namespace uva::smt::tries::mgrams;

namespace uva {
    namespace smt {
        namespace tries {
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
                    BitmapHashCache() : m_num_elems(0), m_data_ptr(NULL) {
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
                    inline void pre_allocate(const size_t num_elems) {
                        if (DO_SANITY_CHECKS && (m_data_ptr != NULL)) {
                            throw Exception("The bitset is already pre-allocated!");
                        }

                        if (num_elems != 0) {
                            m_num_elems = num_elems * __BitmapHashCache::BUCKET_MULTIPLIER_FACTOR;
                            size_t num_bytes = NUM_BYTES_4_BITS(m_num_elems);
                            
                            LOG_DEBUG2 << "Pre-allocating: " << m_num_elems
                                    << " elements, that is " << num_bytes
                                    << " bytes." << END_LOG;
                            
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
                    template<TModelLevel N, typename WordIndexType>
                    inline void add_m_gram(const T_M_Gram<N, WordIndexType> &gram) {
                        LOG_DEBUG2 << "Adding M-gram: " << tokens_to_string(gram) << END_LOG;

                        //Get the bit position
                        uint32_t byte_idx = 0;
                        uint32_t bit_offset_idx = 0;
                        get_bit_pos(gram, byte_idx, bit_offset_idx);

                        LOG_DEBUG2 << "Adding: " << bitset<NUM_BITS_IN_UINT_8>(ON_BIT_ARRAY[bit_offset_idx])
                                << ", to: " << bitset<NUM_BITS_IN_UINT_8>(m_data_ptr[byte_idx]) << END_LOG;

                        //Set the bit on
                        m_data_ptr[byte_idx] |= ON_BIT_ARRAY[bit_offset_idx];
                    }

                    /**
                     * Allows to check if a sub-M-gram of the M-gram of the current level is present in the cache.
                     * @param is_back_off is true if this is a back-off M-gram we need to learn about
                     * @param curr_level the currently considered level of the m-gram
                     * @param query the M-gram query
                     * @return false if the M-gram is not present, otherwise true (the latter means potentially present)
                     */
                    template<bool is_back_off,  TModelLevel curr_level, TModelLevel N, typename WordIndexType>
                    inline bool is_m_gram(const MGramQuery<N, WordIndexType> & query) const {

                        //Depending on the M-gram compute a proper hash
                        TModelLevel begin_idx = 0, end_idx = 0;
                        if (is_back_off) {
                            end_idx = (query.m_gram.level - 2);
                            begin_idx = (query.m_gram.level - 1) - curr_level;
                        } else {
                            end_idx = (query.m_gram.level - 1);
                            begin_idx = query.m_gram.level - curr_level;
                        }

                        uint64_t hash = query.m_gram.sub_hash(begin_idx, end_idx);

                        LOG_DEBUG2 << "The [" << (uint32_t) begin_idx << ", " << (uint32_t) end_idx
                                << "] sub M-gram of: " << tokens_to_string(query.m_gram) << " hash: "
                                << hash << END_LOG;

                        //Get the M-gram hash positions
                        uint32_t byte_idx = 0;
                        uint32_t bit_offset_idx = 0;
                        get_bit_pos(hash, byte_idx, bit_offset_idx);

                        LOG_DEBUG2 << "Returning: " << bitset<NUM_BITS_IN_UINT_8>(m_data_ptr[byte_idx]) << " & "
                                << bitset<NUM_BITS_IN_UINT_8>(ON_BIT_ARRAY[bit_offset_idx]) << END_LOG;

                        //Return the bit on
                        return (m_data_ptr[byte_idx] & ON_BIT_ARRAY[bit_offset_idx]);
                    }

                    /**
                     * Allows to check if the given M-gram is not present in the cache.
                     * @param gram the M-gram to be checked
                     * @return false if the M-gram is not present, otherwise true (the latter means potentially present)
                     */
                    template<TModelLevel N, typename WordIndexType>
                    inline bool is_m_gram(const T_M_Gram<N, WordIndexType> &gram) const {
                        LOG_DEBUG2 << "Checking M-gram: " << tokens_to_string(gram) << END_LOG;

                        //Get the bit position
                        uint32_t byte_idx = 0;
                        uint32_t bit_offset_idx = 0;
                        get_bit_pos(gram, byte_idx, bit_offset_idx);

                        LOG_DEBUG2 << "Returning: " << bitset<NUM_BITS_IN_UINT_8>(m_data_ptr[byte_idx]) << " & "
                                << bitset<NUM_BITS_IN_UINT_8>(ON_BIT_ARRAY[bit_offset_idx]) << END_LOG;

                        //Return the bit on
                        return (m_data_ptr[byte_idx] & ON_BIT_ARRAY[bit_offset_idx]);
                    }

                private:
                    //Stores the number of elements this bitset was pre-allocated for
                    size_t m_num_elems;
                    //Stores the data allocated for the bitset
                    uint8_t * m_data_ptr;

                    /**
                     * Allows to get the bit position for the M-gram
                     * @param hash the M-gram hash get position for
                     * @param byte_idx [out] the M-gram byte index
                     * @param bit_offset_idx [out] the M-gram relative bit index
                     */
                    inline void get_bit_pos(const uint64_t hash, uint32_t & byte_idx, uint32_t & bit_offset_idx) const {
                        LOG_DEBUG2 << "The hash value is: " << hash
                                << ", the number of elements is: " << m_num_elems << END_LOG;
                        
                        //Convert it to the number of elements
                        uint32_t global_bit_idx = hash % m_num_elems;
                        
                        //Convert the global bit index into the byte index and bit offset
                        byte_idx = BYTE_IDX(global_bit_idx);
                        bit_offset_idx = REMAINING_BIT_IDX(global_bit_idx);

                        LOG_DEBUG2 << "The M-gram hash: " << hash << ", byte_idx: " << byte_idx
                                << ", bit_offset_idx: " << bit_offset_idx << END_LOG;
                    }

                    /**
                     * Allows to get the bit position for the M-gram
                     * @param gram the M-gram to get position for
                     * @param byte_idx [out] the M-gram byte index
                     * @param bit_offset_idx [out] the M-gram relative bit index
                     */
                    template<TModelLevel N, typename WordIndexType>
                    inline void get_bit_pos(const T_M_Gram<N, WordIndexType> &gram, uint32_t & byte_idx, uint32_t & bit_offset_idx) const {
                        const uint64_t hash = gram.hash();
                        
                        LOG_DEBUG2 << "The M-gram: " << tokens_to_string(gram)
                                << " hash: " << hash << END_LOG;
                        
                        return get_bit_pos(hash, byte_idx, bit_offset_idx);
                    }
                };
            }
        }
    }
}

#endif	/* BITMAPHASHCACHE_HPP */

