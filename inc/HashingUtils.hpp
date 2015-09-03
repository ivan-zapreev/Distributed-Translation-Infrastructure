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

using namespace std;

namespace uva {
    namespace smt {
        namespace hashing {

            /**
             * The hashing function wrapper base class
             */
            template<typename T>
            class Hash {
            public:
                /**
                 * The type name for the hashing function
                 * @param the string to hash
                 * @return the TShortId - the hash value
                 */
                virtual TShortId operator()(const T &param) const = 0;
            };
            
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

            inline uint32_t computePaulHsiehHash(const char * data, int len) {
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
            inline TShortId computePaulHsiehHash(const string & str) {
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
            inline TShortId computeDjb2Hash(const string & str) {
                TShortId hashVal = 5381;
                int c;
                const char * c_str = str.c_str();

                while ((c = *c_str++)) {
                    hashVal = ((hashVal << 5) + hashVal) + c; /* hash * 33 + c */
                }

                return hashVal;
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

            inline TShortId computePrimesHash(const string & str) {
                TShortId h = 31 /* also prime */;
                const char * c_str = str.c_str();
                while (*c_str) {
                    h = (h * A) ^ (c_str[0] * B);
                    c_str++;
                }
                return h; // or return h % C;
            }

            inline TShortId computeRSHash(const string & str) {
                TShortId b = 378551;
                TShortId a = 63689;
                TShortId hash = 0;

                for (std::size_t i = 0; i < str.length(); i++) {
                    hash = hash * a + str[i];
                    a = a * b;
                }

                return hash;
            }
            
            /**
             * The string hashing function
             * @param param the string to hash
             * @return the resulting hash
             */
            class StringHash : public Hash<string> {
            public:
                virtual TShortId operator()(const string &param) const {
                    return computePaulHsiehHash(param);
                }
            };

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

