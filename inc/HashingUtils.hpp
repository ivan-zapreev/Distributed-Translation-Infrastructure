/* 
 * File:   HashingUtils.hpp
 * Author: zapreevis
 *
 * Created on April 20, 2015, 7:51 PM
 */

#ifndef HASHINGUTILS_HPP
#define	HASHINGUTILS_HPP

#include <string>

using namespace std;

namespace hashing {

    //This is the smallest size which I've tested and it works for the hash without collisions
    typedef int TWordHashSize;
    //This is the hash reference size which should be twice as long as the TWordHashSize
    typedef long int TReferenceHashSize;

    /**
     * This is one of the best known hashing function algorithms (djb2) for the C 
     * strings as reported and described in http://www.cse.yorku.ca/~oz/hash.html
     * @param str the string to hash
     * @return the resulting hash
     */
    inline TWordHashSize computeDjb2Hash(const string & str)
    {
        TWordHashSize hashVal = 5381;
        int c;
        const char * c_str = str.c_str();

        while (c = *c_str++) {
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
     * @param str the word to hash
     * @return the resulting hash
     */    
    #define A 54059 /* a prime */
    #define B 76963 /* another prime */
    #define C 86969 /* yet another prime */
    inline TWordHashSize computePrimesHash(const string & str)
    {
       TWordHashSize h = 31 /* also prime */;
       const char * c_str = str.c_str();
       while (*c_str) {
         h = (h * A) ^ (c_str[0] * B);
         c_str++;
       }
       return h; // or return h % C;
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
    inline TReferenceHashSize cantor(TWordHashSize x, TReferenceHashSize y) {
        return ((x+y)*(x+y+1))/2 + y;
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
    inline void uncantor(const TReferenceHashSize z, TWordHashSize &x, TReferenceHashSize &y){
        const TReferenceHashSize w = floor( ( sqrt( 8 * z + 1 ) - 1 ) / 2);
        const TReferenceHashSize t = ( w * w + w ) / 2;
        y = (z - t);
        x = (TWordHashSize) (w - y);
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
    inline TReferenceHashSize szudzik(TWordHashSize x, TReferenceHashSize y) {
        return ( x >= y ? (y + x + x * x ) : (x + y * y) );
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
    inline void unszudzik(const TReferenceHashSize z, TWordHashSize &x, TReferenceHashSize &y){
        const TReferenceHashSize zrf = floor( sqrt( z ) );
        const TReferenceHashSize zrfs = zrf * zrf;
        const TReferenceHashSize zmzrfs = z - zrfs;
        if( zmzrfs < zrf ) {
            x = zmzrfs;
            y = zrf;
        } else {
            x = zrf;
            y = zmzrfs - zrf;
        }
    }

}

#endif	/* HASHINGUTILS_HPP */

