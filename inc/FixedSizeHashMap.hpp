/* 
 * File:   FixedSizeHashMap.hpp
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
 * Created on December 2, 2015, 12:17 PM
 */

#include <functional>   // std::function 
#include <cmath>        // std::log std::log10
#include <algorithm>    // std::max

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"
#include "ArrayUtils.hpp"

using namespace std;
using namespace uva::smt::utils::array;

#ifndef FIXEDSIZEHASHMAP_HPP
#define	FIXEDSIZEHASHMAP_HPP

namespace uva {
    namespace utils {
        namespace containers {

            /**
             * This class represents a fixed size hash map that stores a pre-defined number of elements.
             * This is a linear probing hash map implementation, the linear probing hash map is currently
             * known to be the fastest hash map there is, see:
             * "Fast and Compact Hash Tables for Integer Keys" by Nikolas Askitis
             * 
             * @param ELEMENT_TYPE the element type, this type is expected to have the following interfase:
             *          1. operator==(const KEY_TYPE &); the comparison operator for the key value
             *          2. static void clear(ELEMENT_TYPE & ); the cleaning method to destroy contents of the element.
             * @param KEY_TYPE the key type for retrieving the element
             * @param IDX_TYPE the index type, is related to the number of elements
             */
            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE = uint32_t>
            class FixedSizeHashMap {
            public:
                typedef ELEMENT_TYPE TElemType;

                //Stores the index that of the non-used element
                static constexpr IDX_TYPE NO_ELEMENT_INDEX = 0;
                //Stores the minimal valid element index
                static constexpr IDX_TYPE MIN_ELEMENT_INDEX = NO_ELEMENT_INDEX + 1;
                //Stores the maximum valid element index
                const IDX_TYPE MAX_ELEMENT_INDEX;

                /**
                 * The basic constructor that allows to instantiate the map for the given number of elements
                 * @param num_elems the number of elements that will be stored in the map
                 */
                explicit FixedSizeHashMap(const double buckets_factor, const IDX_TYPE num_elems) : MAX_ELEMENT_INDEX(num_elems) {
                    //Compute and set the number of buckets and the buckets divider
                    set_number_of_elements(buckets_factor, num_elems);
                    //Set the current number of stored elements to zero
                    m_next_elem_idx = MIN_ELEMENT_INDEX;
                    //Allocate the number of buckets
                    m_buckets = new IDX_TYPE[m_num_buckets];
                    //Allocate the elements, add an extra one, the 0'th 
                    //element will never be used its index is reserved.
                    m_elems = new ELEMENT_TYPE[num_elems + 1];
                }

                /**
                 * Allows to add a new element for the given hash value
                 * @param key_value the key value of the element
                 * @return the reference to the new element
                 */
                ELEMENT_TYPE & add_new_element(const uint_fast64_t key_value) {
                    //Check if the capacity is exceeded.
                    if (m_next_elem_idx > MAX_ELEMENT_INDEX) {
                        THROW_EXCEPTION(string("Used up all the elements, the last ") +
                                string("issued id was: ") + std::to_string(m_next_elem_idx));
                    }

                    //Get the bucket index from the hash
                    uint_fast64_t bucket_idx = get_bucket_idx(key_value);

                    LOG_DEBUG2 << "---------------->Got bucket_idx: " << bucket_idx
                            << " for hash value: " << key_value << END_LOG;
                    //Search for the first empty bucket
                    while (m_buckets[bucket_idx] != NO_ELEMENT_INDEX) {
                        LOG_DEBUG2 << "The bucket: " << bucket_idx <<
                                " is full, skipping to the next." << END_LOG;
                        get_next_bucket_idx(bucket_idx);
                    }

                    LOG_DEBUG2 << "<----------------The first empty bucket index is: "
                            << bucket_idx << END_LOG;

                    //Get the element index and increment
                    const IDX_TYPE elem_idx = m_next_elem_idx++;

                    //Set the bucket to point to the given element
                    m_buckets[bucket_idx] = elem_idx;

                    //Return the element under the index
                    return m_elems[elem_idx];
                }

                /**
                 * Allows to retrieve the element for the given hash value and key
                 * @param key_value the key value of the element
                 * @param key the key value of the element
                 * @return the pointer to the found element or NULL if nothing is found
                 */
                const ELEMENT_TYPE * get_element(const uint_fast64_t key_value, const KEY_TYPE & key) const {
                    //Get the bucket index from the hash
                    uint_fast64_t bucket_idx = get_bucket_idx(key_value);

                    LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << " for hash value: " << key_value << END_LOG;

                    //Search for the first empty bucket
                    while (m_buckets[bucket_idx] != NO_ELEMENT_INDEX) {
                        //Check if the element is equal to the key
                        if (m_elems[m_buckets[bucket_idx]] == key) {
                            //The element is found, return
                            LOG_DEBUG2 << "Found the element index: " << m_buckets[bucket_idx]
                                    << " in bucket: " << bucket_idx << END_LOG;
                            return &m_elems[m_buckets[bucket_idx]];
                        }
                        //The element is not found, move to the next one
                        get_next_bucket_idx(bucket_idx);
                    }

                    LOG_DEBUG2 << "Encountered an empty bucket, the word is unknown!" << END_LOG;

                    return NULL;
                }

                /**
                 * The basic destructor
                 */
                ~FixedSizeHashMap() {
                    if (m_elems != NULL) {
                        //Call the destructors on the allocated objects
                        for (IDX_TYPE idx = MIN_ELEMENT_INDEX; idx < MAX_ELEMENT_INDEX; ++idx) {

                            LOG_DEBUG4 << "Deallocating an element [" << SSTR(idx)
                                    << "]: " << SSTR((void *) &m_elems[idx]) << END_LOG;
                            ELEMENT_TYPE::clear(m_elems[idx]);
                        }
                        //Free the allocated arrays
                        delete[] m_elems;
                        delete[] m_buckets;
                    }
                }

            private:
                //Stores the number of buckets
                uint_fast64_t m_num_buckets;
                //Stores the buckets capacity, there should be at
                //least one empty bucket so the capacity is less then
                //the available number of buckets.
                uint_fast64_t m_buckets_capacity;

                //Stores the current number of stored elements
                IDX_TYPE m_next_elem_idx;

                //Stores the buckets
                IDX_TYPE * m_buckets;
                //Stores the array of reserved elements
                ELEMENT_TYPE * m_elems;

                /**
                 * Sets the number of buckets as a power of two, based on the number of elements
                 * @param buckets_factor the buckets factor that the number of elements will be
                 * multiplied with before converting it into the number of buckets.
                 * @param num_elems the number of elements to compute the buckets for
                 */
                inline void set_number_of_elements(const double buckets_factor, const IDX_TYPE num_elems) {
                    //Do a compulsory assert on the buckets factor

                    ASSERT_CONDITION_THROW((buckets_factor < 1.0), string("buckets_factor: ") +
                            std::to_string(buckets_factor) + string(", must be >= 1.0"));

                    //Compute the number of buckets
                    m_num_buckets = const_expr::power(2, const_expr::ceil(const_expr::log2(buckets_factor * (num_elems + 1))));
                    //Compute the buckets divider
                    m_buckets_capacity = m_num_buckets - 1;

                    ASSERT_CONDITION_THROW((num_elems > m_buckets_capacity), string("Insufficient buckets capacity: ") +
                            std::to_string(m_buckets_capacity) + string(" need at least ") + std::to_string(num_elems));

                    LOG_DEBUG << "FSHM: num_elems: " << num_elems << ", m_num_buckets: " << m_num_buckets
                            << ", m_buckets_capacity: " << m_buckets_capacity << END_LOG;
                }

                /**
                 * This is the mixing function as defined in https://code.google.com/p/fast-hash/
                 * its purpose is to mix the keys so that the keys that would go into the dame
                 * bucket would get spread through out the buckets instead.
                 * @param h the reference to the 64 bit key to mix
                 * @return the reference to the same 64 bit key that has been hashed
                 */
                uint_fast64_t & mix_fasthash(uint_fast64_t & h) const {
                    h ^= h >> 23;
                    h *= 0x2127599bf4325c37ULL;
                    h ^= h >> 47;
                    return h;
                }

                /**
                 * Allows to get the bucket index for the given hash value
                 * @param key_value the key value to compute the bucked index for
                 * @param return the resulting bucket index
                 */
                inline uint_fast64_t get_bucket_idx(uint_fast64_t key_value) const {
                    //Compute the bucket index, note that since m_capacity is the power of two,
                    //we can compute ( hash_value % m_num_buckets ) as ( hash_value & m_capacity )
                    //where m_capacity = ( m_num_buckets - 1);
                    const uint_fast64_t bucket_idx = mix_fasthash(key_value) & m_buckets_capacity;

                    LOG_DEBUG3 << "The mixed key value is: " << key_value
                            << ", bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    ASSERT_SANITY_THROW((bucket_idx > m_buckets_capacity),
                            string("The hash value got a bad bucket index: ") + std::to_string(bucket_idx) +
                            string(", must be within [0, ") + std::to_string(m_buckets_capacity) + "]");

                    return bucket_idx;
                }

                /**
                 * Provides the next bucket index
                 * @param bucket_idx [in/out] the bucket index
                 */
                inline void get_next_bucket_idx(uint_fast64_t & bucket_idx) const {
                    bucket_idx = (bucket_idx + 1) & m_buckets_capacity;
                    LOG_DEBUG3 << "Moving on to the next bucket: " << bucket_idx << END_LOG;
                }
            };

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr IDX_TYPE FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::NO_ELEMENT_INDEX;

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr IDX_TYPE FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::MIN_ELEMENT_INDEX;

        }
    }
}

#endif	/* FIXEDSIZEHASHMAP_HPP */

