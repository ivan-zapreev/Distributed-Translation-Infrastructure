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

#include "Logger.hpp"
#include "Globals.hpp"

#define XXH_NAMESPACE
#include "xxhash.h"   // XXH32 XXH64

#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace hashing {

            /*****************************************************************************************************/
            /***********From https://code.google.com/p/hashfunctions/source/browse/trunk/FNVHash32.h?r=28*********/

            /*****************************************************************************************************/

            inline uint_fast32_t hash32(uint_fast32_t key, uint_fast32_t seed = 2166136261U) {
                uint8_t* bytes = (uint8_t*) (&key);
                uint_fast32_t hash = (16777619U * seed) ^ bytes[0];
                hash = (16777619U * hash) ^ bytes[1];
                hash = (16777619U * hash) ^ bytes[2];
                hash = (16777619U * hash) ^ bytes[3];
                return hash;
            }

            /**
             * Just as boost defines in http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html
             * This is based on the following paper: http://www.cs.rmit.edu.au/~jz/fulltext/jasist-tch.pdf
             * "Methods for Identifying Versioned and Plagiarised Documents"
             * @param key
             * @param seed
             * @return 
             */
            inline uint_fast64_t hash64(uint_fast64_t key, uint_fast64_t seed = 2166136261U) {
                /*
                uint8_t* bytes = (uint8_t*) (&key);
                uint_fast32_t hash = (16777619U * seed) ^ bytes[0];
                hash = (16777619U * hash) ^ bytes[1];
                hash = (16777619U * hash) ^ bytes[2];
                hash = (16777619U * hash) ^ bytes[3];
                hash = (16777619U * hash) ^ bytes[4];
                hash = (16777619U * hash) ^ bytes[5];
                hash = (16777619U * hash) ^ bytes[6];
                hash = (16777619U * hash) ^ bytes[7];
                return hash;
                 */
                return seed ^ (key + 0x9e3779b9 + (seed << 6) + (seed >> 2));
            }

            inline uint_fast32_t hash32_str(const char* data, int len, uint_fast32_t seed = 2166136261U) {
                uint_fast32_t hash = seed;
                for (int32_t i = 0; i != len; ++i) {
                    hash = (16777619U * hash) ^ (uint8_t) (data[i]);
                }
                return hash;
            }

            inline uint_fast32_t hash32_str(const string & token) {
                return hash32_str(token.c_str(), token.length());
            }

            /*****************************************************************************************************/
            /***********From: http://www.boost.org/doc/libs/1_38_0/libs/unordered/examples/fnv1.hpp***************/
            /*****************************************************************************************************/

#ifdef ENVIRONMENT64
            // For 32 bit machines:
            const std::size_t fnv_prime = 16777619u;
            const std::size_t fnv_offset_basis = 2166136261u;
#else
            // For 64 bit machines:
            const std::size_t fnv_prime = 1099511628211u;
            const std::size_t fnv_offset_basis = 14695981039346656037u;
#endif

            inline uint_fast32_t basic_fnv_1(const char * data, uint32_t len) {
                uint_fast32_t hash = fnv_offset_basis;
                for (size_t idx = 0; idx != len; ++idx) {
                    hash *= fnv_prime;
                    hash ^= data[idx];
                }
                return hash;
            };

            inline uint32_t basic_fnv_1(const string & token) {
                return basic_fnv_1(token.c_str(), token.length());
            }

            inline uint_fast32_t basic_fnv_1a(const char * data, uint32_t len) {
                uint_fast32_t hash = fnv_offset_basis;
                for (size_t idx = 0; idx != len; ++idx) {

                    hash ^= data[idx];
                    hash *= fnv_prime;
                }
                return hash;
            };

            inline uint32_t basic_fnv_1a(const string & token) {
                return basic_fnv_1a(token.c_str(), token.length());
            }

            /*****************************************************************************************************/

            //The seed for xxhash
            static const unsigned XXHASH_SEED = 0u;

            inline uint_fast32_t computeXXHash32(const char * data, uint32_t len) {

                return XXH32(data, len, XXHASH_SEED);
            }

            inline uint_fast32_t computeXXHash32(const string & token) {

                return XXH32(token.c_str(), token.length(), XXHASH_SEED);
            }

            inline uint_fast64_t computeXXHash64(const char * data, uint32_t len) {

                return XXH64(data, len, XXHASH_SEED);
            }

            inline uint_fast64_t computeXXHash64(const string & token) {

                return XXH64(token.c_str(), token.length(), XXHASH_SEED);
            }

            /*****************************************************************************************************/

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
#define get16bits(d) ((((uint_fast32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint_fast32_t)(((const uint8_t *)(d))[0]) )
#endif

            inline uint_fast32_t computePaulHsiehHash(const char * data, uint32_t len) {
                uint_fast32_t hash = len, tmp;
                int rem;

                if (len <= 0 || data == NULL) return 0;

                rem = len & 3;
                len >>= 2;

                /* Main loop */
                for (; len != 0; len--) {
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
            inline uint_fast32_t computePaulHsiehHash(const string & str) {

                return computePaulHsiehHash(str.c_str(), str.length());
            }

            /*****************************************************************************************************/

            /**
             * This is one of the best known hashing function algorithms (djb2) for the C 
             * strings as reported and described in http://www.cse.yorku.ca/~oz/hash.html
             * Note: It turned to be not as good as the PrimesHash as resulted in collisions on the test data.
             * Note: The time complexity of this algorithm is also linear in the length of the input word.
             * @param str the string to hash
             * @return the resulting hash
             */
            inline uint_fast32_t computeDjb2Hash(const char * data, uint32_t len) {
                uint_fast32_t hashVal = 5381;

                for (std::size_t i = 0; i != len; ++i) {

                    hashVal = ((hashVal << 5) + hashVal) + data[i]; /* hash * 33 + c */
                }

                return hashVal;
            }

            inline uint_fast32_t computeDjb2Hash(const string & str) {

                return computeDjb2Hash(str.c_str(), str.length());
            }

            /*****************************************************************************************************/

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

            inline uint_fast32_t computePrimesHash(const char * data, uint32_t len) {
                uint_fast32_t h = 31 /* also prime */;
                for (std::size_t i = 0; i != len; ++i) {
                    h = (h * A) ^ (data[i] * B);
                }
                return h; // or return h % C;
            }

            inline uint_fast32_t computePrimesHash(const string & str) {

                return computePrimesHash(str.c_str(), str.length());
            }

            /*****************************************************************************************************/

            inline uint_fast32_t computeRSHash(const char * data, uint32_t len) {
                uint_fast32_t b = 378551;
                uint_fast32_t a = 63689;
                uint_fast32_t hash = 0;

                for (std::size_t i = 0; i != len; ++i) {
                    hash = hash * a + data[i];
                    a = a * b;
                }

                return hash;
            }

            inline uint_fast32_t computeRSHash(const string & str) {
                return computeRSHash(str.c_str(), str.length());
            }

            /*****************************************************************************************************/

            inline uint_fast32_t stupidHash(const char * data, uint32_t len) {
                uint_fast32_t hash = hash32(len);
                for (std::size_t i = 0; i != len; ++i) {
                    hash = (16777619U * hash) ^ data[i];
                }
                return hash;
            }

            inline uint_fast32_t stupidHash(const string & str) {
                return stupidHash(str.c_str(), str.length());
            }

            /*****************************************************************************************************/

            /**
             * The Crap WOW hashing function is taken from:
             * http://floodyberry.com/noncryptohashzoo/CrapWow.html
             * Is expected to be very fast and possess decent properties
             * @param key the pointer to the char array
             * @param len the length of the chaw array
             * @param seed the seed to use
             * @return the hash value
             */
            inline uint32_t crapWowHash(const char *key, uint32_t len, uint32_t seed) {
#if !defined(__LP64__) && !defined(_MSC_VER) && ( defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) )
                // esi = k, ebx = h
                uint32_t hash;
                asm(
                            "leal 0x5052acdb(%%ecx,%%esi), %%esi\n"
                            "movl %%ecx, %%ebx\n"
                            "cmpl $8, %%ecx\n"
                            "jb DW%=\n"
                            "QW%=:\n"
                            "movl $0x5052acdb, %%eax\n"
                            "mull (%%edi)\n"
                            "addl $-8, %%ecx\n"
                            "xorl %%eax, %%ebx\n"
                            "xorl %%edx, %%esi\n"
                            "movl $0x57559429, %%eax\n"
                            "mull 4(%%edi)\n"
                            "xorl %%eax, %%esi\n"
                            "xorl %%edx, %%ebx\n"
                            "addl $8, %%edi\n"
                            "cmpl $8, %%ecx\n"
                            "jae QW%=\n"
                            "DW%=:\n"
                            "cmpl $4, %%ecx\n"
                            "jb B%=\n"
                            "movl $0x5052acdb, %%eax\n"
                            "mull (%%edi)\n"
                            "addl $4, %%edi\n"
                            "xorl %%eax, %%ebx\n"
                            "addl $-4, %%ecx\n"
                            "xorl %%edx, %%esi\n"
                            "B%=:\n"
                            "testl %%ecx, %%ecx\n"
                            "jz F%=\n"
                            "shll $3, %%ecx\n"
                            "movl $1, %%edx\n"
                            "movl $0x57559429, %%eax\n"
                            "shll %%cl, %%edx\n"
                            "addl $-1, %%edx\n"
                            "andl (%%edi), %%edx\n"
                            "mull %%edx\n"
                            "xorl %%eax, %%esi\n"
                            "xorl %%edx, %%ebx\n"
                            "F%=:\n"
                            "leal 0x5052acdb(%%esi), %%edx\n"
                            "xorl %%ebx, %%edx\n"
                            "movl $0x5052acdb, %%eax\n"
                            "mull %%edx\n"
                            "xorl %%ebx, %%eax\n"
                            "xorl %%edx, %%esi\n"
                            "xorl %%esi, %%eax\n"
                            : "=a"(hash), "=c"(len), "=S"(len), "=D"(key)
                            : "c"(len), "S"(seed), "D"(key)
                            : "%ebx", "%edx", "cc"
                            );
                return hash;
#else
#define cwfold( a, b, lo, hi ) { p = (uint32_t)(a) * (uint64_t)(b); lo ^= (uint32_t)p; hi ^= (uint32_t)(p >> 32); }
#define cwmixa( in ) { cwfold( in, m, k, h ); }
#define cwmixb( in ) { cwfold( in, n, h, k ); }
                const uint32_t m = 0x57559429, n = 0x5052acdb, *key4 = (const uint32_t *) key;
                uint32_t h = len, k = len + seed + n;
                uint64_t p;
                while (len >= 8) {
                    cwmixb(key4[0]) cwmixa(key4[1]) key4 += 2;
                    len -= 8;
                }
                if (len >= 4) {
                    cwmixb(key4[0]) key4 += 1;
                    len -= 4;
                }
                if (len) {
                    cwmixa(key4[0] & ((1 << (len * 8)) - 1))
                }
                cwmixb(h ^ (k + n))
                return k ^ h;
#endif
            }

            /*****************************************************************************************************/

            //-----------------------------------------------------------------------------
            // MurmurHash2, 64-bit versions, by Austin Appleby

            // The same caveats as 32-bit MurmurHash2 apply here - beware of alignment 
            // and endian-ness issues if used across multiple platforms.

            // 64-bit hash for 64-bit platforms

            inline uint64_t MurmurHash64A(const void * key, std::size_t len, uint64_t seed) {
                const uint64_t m = 0xc6a4a7935bd1e995ULL;
                const int r = 47;

                uint64_t h = seed ^ (len * m);

#if defined(__arm) || defined(__arm__)
                const size_t ksize = sizeof (uint64_t);
                const unsigned char * data = (const unsigned char *) key;
                const unsigned char * end = data + (std::size_t)(len / 8) * ksize;
#else
                const uint64_t * data = (const uint64_t *) key;
                const uint64_t * end = data + (len / 8);
#endif

                while (data != end) {
#if defined(__arm) || defined(__arm__)
                    uint64_t k;
                    memcpy(&k, data, ksize);
                    data += ksize;
#else
                    uint64_t k = *data++;
#endif

                    k *= m;
                    k ^= k >> r;
                    k *= m;

                    h ^= k;
                    h *= m;
                }

                const unsigned char * data2 = (const unsigned char*) data;

                switch (len & 7) {
                    case 7: h ^= uint64_t(data2[6]) << 48;
                    case 6: h ^= uint64_t(data2[5]) << 40;
                    case 5: h ^= uint64_t(data2[4]) << 32;
                    case 4: h ^= uint64_t(data2[3]) << 24;
                    case 3: h ^= uint64_t(data2[2]) << 16;
                    case 2: h ^= uint64_t(data2[1]) << 8;
                    case 1: h ^= uint64_t(data2[0]);
                        h *= m;
                };

                h ^= h >> r;
                h *= m;
                h ^= h >> r;

                return h;
            }


            // 64-bit hash for 32-bit platforms

            inline uint64_t MurmurHash64B(uint64_t seed, const void * key, std::size_t len) {
                const unsigned int m = 0x5bd1e995;
                const int r = 24;

                unsigned int h1 = seed ^ len;
                unsigned int h2 = 0;

#if defined(__arm) || defined(__arm__)
                size_t ksize = sizeof (unsigned int);
                const unsigned char * data = (const unsigned char *) key;
#else
                const unsigned int * data = (const unsigned int *) key;
#endif

                unsigned int k1, k2;
                while (len >= 8) {
#if defined(__arm) || defined(__arm__)
                    memcpy(&k1, data, ksize);
                    data += ksize;
                    memcpy(&k2, data, ksize);
                    data += ksize;
#else
                    k1 = *data++;
                    k2 = *data++;
#endif

                    k1 *= m;
                    k1 ^= k1 >> r;
                    k1 *= m;
                    h1 *= m;
                    h1 ^= k1;
                    len -= 4;

                    k2 *= m;
                    k2 ^= k2 >> r;
                    k2 *= m;
                    h2 *= m;
                    h2 ^= k2;
                    len -= 4;
                }

                if (len >= 4) {
#if defined(__arm) || defined(__arm__)
                    memcpy(&k1, data, ksize);
                    data += ksize;
#else
                    k1 = *data++;
#endif
                    k1 *= m;
                    k1 ^= k1 >> r;
                    k1 *= m;
                    h1 *= m;
                    h1 ^= k1;
                    len -= 4;
                }

                switch (len) {
                    case 3: h2 ^= ((unsigned char*) data)[2] << 16;
                    case 2: h2 ^= ((unsigned char*) data)[1] << 8;
                    case 1: h2 ^= ((unsigned char*) data)[0];
                        h2 *= m;
                };

                h1 ^= h2 >> 18;
                h1 *= m;
                h2 ^= h1 >> 22;
                h2 *= m;
                h1 ^= h2 >> 17;
                h1 *= m;
                h2 ^= h1 >> 19;
                h2 *= m;

                uint64_t h = h1;

                h = (h << 32) | h2;

                return h;
            }

            /*****************************************************************************************************/

            /**
             * The function used to compute hash in the application, uses one of the specific hashing functions above.
             * @param data the data to hash
             * @param len the length of the data to hash
             * @return the resulting hash.
             */
            inline uint_fast64_t compute_hash(const char * data, uint32_t len, const uint64_t seed = 16777619U) {
                return MurmurHash64A(data, len, seed);
            }

            /**
             * The function used to compute hash in the application, uses one of the specific hashing functions above.
             * @param token the string to hash
             * @return the resulting hash.
             */
            inline uint_fast64_t compute_hash(const TextPieceReader & token, const uint64_t seed = 16777619U) {
                return compute_hash(token.get_begin_c_str(), token.length(), seed);
            }

            /**
             * The function used to compute hash in the application, uses one of the specific hashing functions above.
             * @param token the token to compute hash for
             * @return the resulting hash.
             */
            inline uint_fast64_t compute_hash(const string & token, const uint64_t seed = 16777619U) {
                return compute_hash(token.c_str(), token.length(), seed);
            }

            /*****************************************************************************************************/

            /**
             * Allows to compute the hash limited by the given value
             * @param data the data to hash
             * @param len the length of the data to hash
             * @param limit the upper limit of the resulting hash (non-reachable)
             * @return the resulting hash < limit
             */
            inline uint_fast32_t compute_hash(const uint32_t limit, const char * data, uint32_t len, const uint64_t seed = 16777619U) {
                const uint_fast64_t hash = compute_hash(data, len, seed);
                return static_cast<uint_fast32_t>( hash - (hash / limit) * limit);
            }

            /**
             * Allows to compute the hash limited by the given value
             * @param token the string to hash
             * @param limit the upper limit of the resulting hash (non-reachable)
             * @return the resulting hash < limit
             */
            inline uint_fast32_t compute_hash(const uint32_t limit, const TextPieceReader & token, const uint64_t seed = 16777619U) {
                return compute_hash(limit, token.get_begin_c_str(), token.length(), seed);
            }

            /**
             * Allows to compute the hash limited by the given value
             * @param token the string to hash
             * @param limit the upper limit of the resulting hash (non-reachable)
             * @return the resulting hash < limit
             */
            inline uint_fast32_t compute_hash(const uint32_t limit, const string & token, const uint64_t seed = 16777619U) {
                return compute_hash(limit, token.c_str(), token.length(), seed);
            }

            /*****************************************************************************************************/

            /**
             * Allows to combine two hashes into one
             * @param hash_one the first hash value
             * @param hash_two the second hash value
             * @return the resulting combines hash value
             */
            inline uint_fast64_t combine_hash(const uint_fast64_t hash_one, const uint_fast64_t hash_two) {
                return hash64(hash_one, hash_two);
            }

            /*****************************************************************************************************/

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

