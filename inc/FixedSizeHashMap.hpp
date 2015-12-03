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
             *          1. operator==(const KEY_TYPE &); the comparison operator for the key value
             *          2. static void clear(ELEMENT_TYPE & ); the cleaning method to destroy contents of the element.
             * @param KEY_TYPE the key type for retrieving the element
             * @param IDX_TYPE the index type, is related to the number of elements
             */
            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE = uint32_t>
            class FixedSizeHashMap {
            public:
                typedef ELEMENT_TYPE TElemType;
                //Stores the number of bucket steps we are going to use, must be a power of two
                static constexpr uint_fast32_t NUM_BUCKET_STEPS = 256;
                //Stores the bucket step index divider
                static constexpr uint_fast32_t STEP_IDX_DIVIDER = NUM_BUCKET_STEPS - 1;
                //Stores the step taken to go to the next bucket
                static constexpr uint_fast64_t BUCKET_STEPS[NUM_BUCKET_STEPS] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 1613, 1619, 1621};

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
                 * @param hash_value the hash value of the element
                 * @return the reference to the new element
                 */
                ELEMENT_TYPE & add_new_element(const uint_fast64_t hash_value) {
                    //Check if the capacity is exceeded.
                    if (m_next_elem_idx > MAX_ELEMENT_INDEX) {
                        THROW_EXCEPTION(string("Used up all the elements, the last ") +
                                string("issued id was: ") + std::to_string(m_next_elem_idx));
                    }

                    //Get the bucket index from the hash
                    uint_fast64_t bucket_idx = get_bucket_idx(hash_value);

                    LOG_DEBUG2 << "---------------->Got bucket_idx: " << bucket_idx
                            << " for hash value: " << hash_value << END_LOG;
                    //Search for the first empty bucket
                    uint_fast32_t attempt = 0;
                    while (m_buckets[bucket_idx] != NO_ELEMENT_INDEX) {
                        LOG_DEBUG2 << "The bucket: " << bucket_idx <<
                                " is full, skipping to the next." << END_LOG;
                        get_next_bucket_idx(attempt, bucket_idx);
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
                 * @param hash_value the hash value of the element
                 * @param key the key value of the element
                 * @return the pointer to the found element or NULL if nothing is found
                 */
                const ELEMENT_TYPE * get_element(const uint_fast64_t hash_value, const KEY_TYPE & key) const {
                    //Get the bucket index from the hash
                    uint_fast64_t bucket_idx = get_bucket_idx(hash_value);

                    LOG_DEBUG2 << "Got bucket_idx: " << bucket_idx << " for hash value: " << hash_value << END_LOG;

                    //Search for the first empty bucket
                    uint_fast32_t attempt = 0;
                    while (m_buckets[bucket_idx] != NO_ELEMENT_INDEX) {
                        //Check if the element is equal to the key
                        if (m_elems[m_buckets[bucket_idx]] == key) {
                            //The element is found, return
                            LOG_DEBUG2 << "Found the element index: " << m_buckets[bucket_idx]
                                    << " in bucket: " << bucket_idx << END_LOG;
                            return &m_elems[m_buckets[bucket_idx]];
                        }
                        //The element is not found, move to the next one
                        get_next_bucket_idx(attempt, bucket_idx);
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
                 * Allows to get the bucket index for the given hash value
                 * @param hash_value the hash value to compute the bucked index for
                 * @param return the resulting bucket index
                 */
                inline uint_fast64_t get_bucket_idx(const uint_fast64_t hash_value) const {
                    //Compute the bucket index, note that since m_capacity is the power of two,
                    //we can compute ( hash_value % m_num_buckets ) as ( hash_value & m_capacity )
                    //where m_capacity = ( m_num_buckets - 1);
                    const uint_fast64_t bucket_idx = hash_value & m_buckets_capacity;

                    LOG_DEBUG1 << "The hash value is: " << hash_value << ", bucket_idx: " << SSTR(bucket_idx) << END_LOG;

                    //If the sanity check is on then check on that the id is within the range
                    ASSERT_SANITY_THROW((bucket_idx > m_buckets_capacity),
                            string("The hash value got a bad bucket index: ") + std::to_string(bucket_idx) +
                            string(", must be within [0, ") + std::to_string(m_buckets_capacity) + "]");

                    return bucket_idx;
                }

                /**
                 * Provides the next bucket index
                 * @param index the index for getting the next step id
                 * @param bucket_idx [in/out] the bucket index
                 */
                inline void get_next_bucket_idx(uint_fast32_t & index, uint_fast64_t & bucket_idx) const {
                    index++;
                    index &= STEP_IDX_DIVIDER;
                    bucket_idx = (bucket_idx + BUCKET_STEPS[index]) & m_buckets_capacity;
                    LOG_DEBUG3 << "Moving on to the next bucket: " << bucket_idx << END_LOG;
                }
            };

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr uint_fast64_t FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::BUCKET_STEPS[NUM_BUCKET_STEPS];

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr uint_fast32_t FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::NUM_BUCKET_STEPS;

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr uint_fast32_t FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::STEP_IDX_DIVIDER;

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr IDX_TYPE FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::NO_ELEMENT_INDEX;

            template<typename ELEMENT_TYPE, typename KEY_TYPE, typename IDX_TYPE>
            constexpr IDX_TYPE FixedSizeHashMap<ELEMENT_TYPE, KEY_TYPE, IDX_TYPE>::MIN_ELEMENT_INDEX;

        }
    }
}

#endif	/* FIXEDSIZEHASHMAP_HPP */

