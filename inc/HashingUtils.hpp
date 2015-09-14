/* 
 * File:   HashingUtils.hpp
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
 * Created on April 20, 2015, 7:51 PM
 */

#ifndef HASHINGUTILS_HPP
#define	HASHINGUTILS_HPP

#include <string>     // std::string
#include <cmath>      // std::floor, std::sqrt
#include <stdint.h>   // srd::uint32_t
#include <functional> // std::function

#include "Logger.hpp"
#include "Globals.hpp"

#define XXH_NAMESPACE
#include "xxhash.h"   // XXH32 XXH64

using namespace std;

namespace uva {
    namespace smt {
        namespace hashing {

            //The seed for xxhash
            static const unsigned XXHASH_SEED = 0u;

            inline uint32_t computeXXHash32(const char * data, uint32_t len) {
                return XXH32(data, len, XXHASH_SEED);
            }

            inline uint64_t computeXXHash64(const char * data, uint32_t len) {
                return XXH64(data, len, XXHASH_SEED);
            }

            /**
             * The string hashing functions: 
             * computePaulHsiehHash - This one showed the worst speed on a test run
             * computeDjb2Hash - This one showed medium speed on a test run
             * computePrimesHash - This one showed medium speed on a test run
             * computeRSHash - This one showed the best speed on a test run
             * 
             * Note that the XXHASH should be the best with respect to everything, see:
             * https://github.com/Cyan4973/xxHash
             * 
             * Yet it is not even on a 64 bit machine with XXH64 it is beated by RSHash!
             * At least the hash based trie performs faster (200 vs 250 CPU seconds)
             * on a 20 Gb model with 100.000.000 queries. So for us XXHASH is not the best.
             */

            /**
             * The following is the Paul Hsieh implementation of a string hashing function
             * This one seems to be very efficient in computation time and has good distribution:
             * http://www.azillionmonkeys.com/qed/hash.html
             */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

            inline uint32_t computePaulHsiehHash(const char * data, uint32_t len) {
                uint32_t hash = len, tmp;
                int rem;

                if (len <= 0 || data == NULL) return 0;

                rem = len & 3;
                len >>= 2;

                /* Main loop */
                for (; len > 0; len--) {
                    hash += get16bits(data);
                    tmp = (get16bits(data + 2) << 11) ^ hash;
                    hash = (hash << 16) ^ tmp;
                    data += 2 * sizeof (uint16_t);
                    hash += hash >> 11;
                }

                /* Handle end cases */
                switch (rem) {
                    case 3: hash += get16bits(data);
                        hash ^= hash << 16;
                        hash ^= ((signed char) data[sizeof (uint16_t)]) << 18;
                        hash += hash >> 11;
                        break;
                    case 2: hash += get16bits(data);
                        hash ^= hash << 11;
                        hash += hash >> 17;
                        break;
                    case 1: hash += (signed char) *data;
                        hash ^= hash << 10;
                        hash += hash >> 1;
                }

                /* Force "avalanching" of final 127 bits */
                hash ^= hash << 3;
                hash += hash >> 5;
                hash ^= hash << 4;
                hash += hash >> 17;
                hash ^= hash << 25;
                hash += hash >> 6;

                return hash;
            }

            /**
             * The simple function wrapper for the Paul Hsieh Hashing function
             * @param str the string to hash
             * @return the hash value
             */
            inline uint32_t computePaulHsiehHash(const string & str) {
                return computePaulHsiehHash(str.c_str(), str.length());
            }

            /**
             * This is one of the best known hashing function algorithms (djb2) for the C 
             * strings as reported and described in http://www.cse.yorku.ca/~oz/hash.html
             * Note: It turned to be not as good as the PrimesHash as resulted in collisions on the test data.
             * Note: The time complexity of this algorithm is also linear in the length of the input word.
             * @param str the string to hash
             * @return the resulting hash
             */
            inline uint32_t computeDjb2Hash(const char * data, uint32_t len) {
                uint32_t hashVal = 5381;

                for (std::size_t i = 0; i < len; i++) {
                    hashVal = ((hashVal << 5) + hashVal) + data[i]; /* hash * 33 + c */
                }

                return hashVal;
            }

            inline uint32_t computeDjb2Hash(const string & str) {
                return computeDjb2Hash(str.c_str(), str.length());
            }


            /**
             * This is a hash function found online 
             * http://stackoverflow.com/questions/8317508/hash-function-for-a-string
             * It's origin is unknown but it proves to work perfect (without collisions)
             * on both test sets! So I do not need to complicate a hash map to a
             * multi-map for now!
             * Note: The time complexity is linear in the length of the word.
             * Note: There are no observed collisions up until now.
             * Note: But it is not yet known if this hash is collision free.
             * @param str the word to hash
             * @return the resulting hash
             */
#define A 54059 /* a prime */
#define B 76963 /* another prime */
#define C 86969 /* yet another prime */

            inline uint32_t computePrimesHash(const char * data, uint32_t len) {
                uint32_t h = 31 /* also prime */;
                for (std::size_t i = 0; i < len; i++) {
                    h = (h * A) ^ (data[i] * B);
                }
                return h; // or return h % C;
            }

            inline uint32_t computePrimesHash(const string & str) {
                return computePrimesHash(str.c_str(), str.length());
            }

            inline uint32_t computeRSHash(const char * data, uint32_t len) {
                uint32_t b = 378551;
                uint32_t a = 63689;
                uint32_t hash = 0;

                for (std::size_t i = 0; i < len; i++) {
                    hash = hash * a + data[i];
                    a = a * b;
                }

                return hash;
            }

            inline uint32_t computeRSHash(const string & str) {
                return computeRSHash(str.c_str(), str.length());
            }

            inline uint64_t computeHash(const char * data, uint32_t len) {
                return computeDjb2Hash(data, len);
            }

            /**
             * This function will combine two word references to get one hash map
             * N-gram level reference. This is a cantor function used for pairing.
             * http://en.wikipedia.org/wiki/Pairing_function#Cantor_pairing_function
             * WARNING: does not work good with big numbers! Overflows!
             * @param x the key word reference
             * @param y the previous context
             * @return the context reference for the next N-gram level
             */
            inline TLongId cantor(TLongId x, TLongId y) {
                return ((x + y)*(x + y + 1)) / 2 + y;
            }

            /**
             * This function will invert the cuntor encoded context and split it into two things
             * The previous word and the previous context.  This is an inverse cantor function used for unpairing.
             * http://en.wikipedia.org/wiki/Pairing_function#Cantor_pairing_function
             * WARNING: does not work good with big numbers! Overflows!
             * @param z the current context id
             * @param x the previous word in the context
             * @param y the context of the previous word
             */
            inline void uncantor(const TLongId z, TShortId &x, TLongId &y) {
                const TLongId w = floor((sqrt(8 * z + 1) - 1) / 2);
                const TLongId t = (w * w + w) / 2;
                y = (z - t);
                x = (TShortId) (w - y);
            }

            /**
             * This function will combine two word references to get one hash map
             * N-gram level reference. This is a Szudzik's function used for pairing.
             * http://szudzik.com/ElegantPairing.pdf
             * This function is more efficient that than of Cantor as it uses indexes densely
             * the result of szudzik(N,M) is <= 2*max(M,N), so the risk of owerflows is much smaller!
             * @param x the key word reference
             * @param y the previous context
             * @return the context reference for the next N-gram level
             */
            inline TLongId szudzik(TLongId x, TLongId y) {
                return ( x >= y ? (y + x + x * x) : (x + y * y));
            }

            /**
             * This function will invert the Szudzik's encoded context and split it into two things
             * The previous word and the previous context.  This is an inverse Szudzik's function used for unpairing.
             * http://szudzik.com/ElegantPairing.pdf
             * WARNING: does not work good with big numbers! Owerflows!
             * @param z the current context id
             * @param x the previous word in the context
             * @param y the context of the previous word
             */
            inline void unszudzik(const TLongId z, TShortId &x, TLongId &y) {
                const TLongId zrf = floor(sqrt(z));
                const TLongId zrfs = zrf * zrf;
                const TLongId zmzrfs = z - zrfs;
                if (zmzrfs < zrf) {
                    x = zmzrfs;
                    y = zrf;
                } else {
                    x = zrf;
                    y = zmzrfs - zrf;
                }
            }

        }
    }
}
#endif	/* HASHINGUTILS_HPP */

