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
             * This class represents a fixed size hash map that stores a pre-defined number of elements
             * @param ELEMENT_TYPE the element type, this type is expected to have the following interfase:
             *          1. bool is_full(); method that allows to check if the lement is used or not
             *          2. operator==(const KEY_TYPE &); the comparison operator for the key value
             *          3. static void clear(ELEMENT_TYPE & ); the cleaning method to destroy contents of the element.
             * @param KEY_TYPE the key type for retrieving the element
             */
            template<typename ELEMENT_TYPE, typename KEY_TYPE>
            class FixedSizeHashMap {
            public:
                typedef ELEMENT_TYPE TElemType;

                /**
                 * The basic constructor that allows to instantiate the map for the given number of elements
                 * @param num_elems the number of elements that will be stored in the map
                 */
                explicit FixedSizeHashMap(const double buckets_factor, const size_t num_elems) {
                    //Compute and set the number of buckets and the buckets divider
                    set_number_of_elements(buckets_factor, num_elems);
                    //Set the current number of stored elements to zero
                    m_size = 0;
                    //Allocate the elements
                    m_ptr = new ELEMENT_TYPE[m_num_buckets];
                }

                /**
                 * Allows to add a new element for the given hash value
                 * @param hash_value the hash value of the element
                 * @return the reference to the new element
                 */
                ELEMENT_TYPE & add_new_element(const uint_fast64_t hash_value) const {
                    //Check if the capacity is exceeded.
                    if (m_size < m_capacity) {
                        //Get the bucket index from the hash
                        uint_fast64_t bucket_idx = get_bucket_idx(hash_value);

                        LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << " for hash value: " << hash_value << END_LOG;

                        //Search for the first empty bucket
                        while (m_ptr[bucket_idx].is_full()) {
                            LOG_DEBUG2 << "The bucket: " << bucket_idx <<
                                    " is full, skipping to the next." << END_LOG;
                            get_next_bucket_idx(bucket_idx);
                        }

                        LOG_DEBUG2 << "The first empty bucket index is: " << bucket_idx << END_LOG;

                        return m_ptr[bucket_idx];
                    } else {
                        THROW_EXCEPTION(string("Exceeded the maximum allowed ") +
                                string("capacity: ") + std::to_string(m_capacity));
                    }
                }

                /**
                 * Allows to retrieve the element for the given hash value and key
                 * @param hash_value the hash value of the element
                 * @param key the key value of the element
                 * @return the pointer to the found element or NULL if nothing is found
                 */
                const ELEMENT_TYPE * get_element(const uint_fast64_t hash_value, const KEY_TYPE & key) const {
                    //Get the bucket index from the hash
                    uint_fast64_t bucket_idx = get_bucket_idx(hash_value);

                    LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << " for hash value: " << hash_value << END_LOG;

                    //Search for the first empty bucket
                    while (m_ptr[bucket_idx].is_full()) {
                        //Check if the element is equal to the key
                        if (m_ptr[bucket_idx] == key) {
                            //The element is found, return
                            LOG_DEBUG2 << "Found the lement in bucket: " << bucket_idx << END_LOG;
                            return &m_ptr[bucket_idx];
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
                    if (m_ptr != NULL) {
                        //Call the destructors on the allocated objects
                        for (size_t idx = 0; idx < m_capacity; ++idx) {
                            LOG_DEBUG4 << "Deallocating an element [" << SSTR(idx)
                                    << "]: " << SSTR((void *) &m_ptr[idx]) << END_LOG;
                            ELEMENT_TYPE::clear(m_ptr[idx]);
                        }
                        //Free the allocated arrays
                        delete[] m_ptr;
                    }
                }

            private:
                //Stores the number of buckets
                size_t m_num_buckets;
                //Stores the buckets divider
                size_t m_capacity;
                //Stores the current number of stored elements
                size_t m_size;
                //Stores the array of reserved elements
                ELEMENT_TYPE * m_ptr;

                /**
                 * Sets the number of buckets as a power of two, based on the number of elements
                 * @param buckets_factor the buckets factor that the number of elements will be
                 * multiplied with before converting it into the number of buckets.
                 * @param num_elems the number of elements to compute the buckets for
                 */
                inline void set_number_of_elements(const double buckets_factor, const size_t num_elems) {
                    //Do a compulsory assert on the buckets factor
                    ASSERT_CONDITION_THROW((buckets_factor < 1.0), "buckets_factor must be >= 1.0");
                    //Compute the number of buckets
                    m_num_buckets = const_expr::power(2, const_expr::ceil(const_expr::log2(buckets_factor * (num_elems + 1))));
                    //Compute the buckets divider
                    m_capacity = m_num_buckets - 1;

                    LOG_DEBUG << "num_elems: " << num_elems << ", m_num_buckets: " << m_num_buckets
                            << ", m_bucket_divider: " << m_capacity << END_LOG;
                }

                /**
                 * Allows to get the bucket index for the given hash value
                 * @param hash_value the hash value to compute the bucked index for
                 * @param return the resulting bucket index
                 */
                inline uint_fast64_t get_bucket_idx(const uint_fast64_t hash_value) const {
                    //Compute the bucket index, note that since m_capacity is the power of two,
                    //we can compute ( hash_value % m_num_buckets ) as ( hash_value & m_capacity )
                    //where m_capacity = ( m_num_buckets - 1);
                    const uint_fast64_t bucket_idx = hash_value & m_capacity;

                    LOG_DEBUG1 << "The hash value is: " << hash_value << ", bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    ASSERT_SANITY_THROW((bucket_idx > m_capacity),
                            string("The hash value got a bad bucket index: ") + std::to_string(bucket_idx) +
                            string(", must be within [0, ") + std::to_string(m_capacity) + "]");

                    return bucket_idx;
                }

                /**
                 * Provides the next bucket index
                 * @param bucket_idx [in/out] the bucket index
                 */
                inline void get_next_bucket_idx(uint_fast64_t & bucket_idx) const {
                    bucket_idx = (bucket_idx + 1) & m_capacity;
                    LOG_DEBUG3 << "Moving on to the next bucket: " << bucket_idx << END_LOG;
                }

            };
        }
    }
}

#endif	/* FIXEDSIZEHASHMAP_HPP */

