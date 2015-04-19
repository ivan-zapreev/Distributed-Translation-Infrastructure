/* 
 * File:   HashMapTrie.h
 * Author: zapreevis
 *
 * Created on April 18, 2015, 11:42 AM
 */

#ifndef HASHMAPTRIE_HPP
#define	HASHMAPTRIE_HPP

#include "ITrie.hpp"
#include <cmath>

/**
 * We actually have several choices:
 * 
 * Continue to use <ext/hash_map.h> and use -Wno-deprecated to stop the warning
 * 
 * Use <tr1/unordered_map> and std::tr1::unordered_map
 * 
 * Use <unordered_map> and std::unordered_map and -std=c++0x
 * 
 * We will need to test which one runs better, it is an unordered_map for now.
 * http://www.cplusplus.com/reference/unordered_map/unordered_map/
 */

#include <utility>      // std::pair, std::make_pair
#include <tr1/tuple>
#include <tr1/unordered_map>

using namespace std;
using std::tr1::unordered_map;
using std::tr1::tuple;

/**
 * This is a HashMpa based ITrie interface implementation class.
 * Note 1: This implementation uses the unsigned long for the hashes it is not optimal
 * Note 2: the unordered_map might be not as efficient as a hash_map with respect to memory usage but it is supposed to be faster
 * 
 * This implementation is chosen because it resembles the ordered array implementation from:
 *      "Faster and Smaller N -Gram Language Models"
 *      Adam Pauls Dan Klein
 *      Computer Science Division
 *      University of California, Berkeley
 * 
 * and unordered_maps showed good performance in:
 *      "Efficient in-memory data structures for n-grams indexing"
 *       Daniel Robenek, Jan Platoˇs, and V ́aclav Sn ́aˇsel
 *       Department of Computer Science, FEI, VSB – Technical University of Ostrava
 *       17. listopadu 15, 708 33, Ostrava-Poruba, Czech Republic
 *       {daniel.robenek.st, jan.platos, vaclav.snasel}@vsb.cz
 * 
 */
template<unsigned int N>
class HashMapTrie : public ITrie {
public:
    HashMapTrie();
    
    void addWords(vector<string> &tokens);
    void addNGram(vector<string> &tokens, const int idx, const int n );
    unsigned int getNGramLevel() {return N;};
    
    virtual ~HashMapTrie();
    
private:
    //The amount of memory dedicated for storing frequency
    typedef unsigned short int TFrequencySize;
    //This is the smallest size which I've tested and it works for the hash without collisions
    typedef int TWordHashSize;
    //This is the hash reference size which should be twice as long as the TWordHashSize
    typedef long int TReferenceHashSize;
    //A tuple storing a word and its frequency
    typedef pair<string, TWordHashSize> TWordEntryPair;
    //The N-trie level entry tuple for a word
    typedef unordered_map<TReferenceHashSize, TFrequencySize> TNTrieEntryPair;
    
    //The map storing the dictionary
    unordered_map<TWordHashSize, TWordEntryPair> words;
    //The map storing n-tires for n>=2 and <= N
    unordered_map<TWordHashSize, TNTrieEntryPair > data[N-1];

    /**
     * The copy constructor, is made private as we do not intend to copy this class objects
     * @param orig the object to copy from
     */
    HashMapTrie(const HashMapTrie& orig);

    /**
     * This function has pure debug purpose, so its impl is ugly
     * This implementation delivers quite a few hash collisions!
     * This is not something I want to have at the moment as then
     * the data structure will need to be more complex. So this
     * function will not be used.
     * @param tokens the tokens to print
     * @param hash the begin of the n-gram
     * @param n the number of elements in the n-gram
     */
    void printDebugNGram(vector<string> &tokens, const int hash, const int n );

    /**
     * This is one of the best known hashing function algorithms (djb2) for the C 
     * strings as reported and described in http://www.cse.yorku.ca/~oz/hash.html
     * @param str the string to hash
     * @return the resulting hash
     */
    static inline TWordHashSize computeDjb2Hash(const string & str)
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
    static inline TWordHashSize computePrimesHash(const string & str)
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
    static inline TReferenceHashSize cantor(TWordHashSize x, TReferenceHashSize y) {
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
    static inline void uncantor(const TReferenceHashSize z, TWordHashSize &x, TReferenceHashSize &y){
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
    static inline TReferenceHashSize szudzik(TWordHashSize x, TReferenceHashSize y) {
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
    static inline void unszudzik(const TReferenceHashSize z, TWordHashSize &x, TReferenceHashSize &y){
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
    
    /**
     * This function computes the hash of the word
     * @param str the word to hash
     * @return the resulting hash
     */
    static inline TWordHashSize computeHash(const string & str) {
        return computePrimesHash(str);
    }
  
    /**
     * Computes the N-Gram context using the previous context and the current word hash
     * @param hash the current word hash
     * @param context the previous context
     * @return the resulting context
     */
    static inline TReferenceHashSize createContext(TWordHashSize hash, TReferenceHashSize context) {
        return szudzik(hash, context);
    }
    
    /**
     * This function dissolves the given Ngram context (for N>=2) into a sub-word
     * hash and a sub-context: c(w_n) is defined by hash(w_n) and c(w_(n-1))
     * @param context the given context to dissolve 
     * @param subWord the sub-work
     * @param subContext the sub-context
     */
    static inline void dessolveContext(const TReferenceHashSize context, TWordHashSize &subWord, TReferenceHashSize &subContext){
        unszudzik(context,subWord,subContext);
    }

};

#endif	/* HASHMAPTRIE_HPP */

