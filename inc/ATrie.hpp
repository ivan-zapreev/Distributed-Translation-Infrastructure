/* 
 * File:   ITries.h
 * Author: zapreevis
 *
 * Created on April 18, 2015, 11:38 AM
 */

#ifndef ITRIES_HPP
#define	ITRIES_HPP

#include <vector>
#include <string>

#include "Globals.hpp"
#include "Exceptions.hpp"

using namespace std;

namespace tries {

    //The amount of memory dedicated for storing frequency
    typedef unsigned short int TFrequencySize;


    //This typedef defines a frequency result type, which is a reference to
    //an array of N frequency values. Note that the value with index [0] will
    //contain the frequency for the 1-gram en so forth.
    template<TTrieSize N> struct SFrequencyResult {
        TFrequencySize result[N];
    };
   
    
    /**
     * This is a common abstract class for all possible Trie implementations
     * The purpose of having this as a template class is performance optimization.
     * It is a template class that has two template parameters:
     * @param N - the maximum level of the considered N-gram, i.e. the N value
     * @param doCache - the indicative flag that asks the child class to, if possible,
     *                  cache the queries.
     */
    template<TTrieSize N, bool doCache>
    class ATrie {
    public:
        
        /**
         * This method adds a words to the trie
         * @param tokens the array of tokens/words to add the trie from
         */
        virtual void addWords(vector<string> &tokens) = 0;

        /**
         * This method adds a new n-gram into the trie
         * @param tokens the array of tokens to add the trie from
         * @param idx the index to start with
         * @param n the value of "n" for the n-gram (the number of elements in the n-gram).
         */
        virtual void addNGram(vector<string> &tokens, const int idx, const int n ) = 0;

        /**
         * Returns the maximum length of the considered N-Grams
         * @return the maximum length of the considered N-Grams
         */
        unsigned int getNGramLevel() const {return N;}

        /**
         * Allows to test if the given Trie implementation has internal query caches
         * @return true if the cache exists otherwise false
         */
        virtual bool doesQueryCache() const { return doCache;}

        /**
         * This function will query the trie for all the n-grams finishing with the given word.
         * This function is to be used when no query result caching is done.
         * @param word the word that is begin queued
         * @param wrap the reference of an array into which the result values will 
         *               be put. The result[0] will contain frequency for 0-gram etc.
         * @throws Exception in case this function is used when query result caching is done.
         */
        virtual void queryWordFreqs(const string & word,  SFrequencyResult<N> & result ) throw (Exception) = 0;

        /**
         * This function will query the trie for all the n-grams finishing with the given word.
         * This function is to be used when result caching is done.
         * @param word the word that is begin queued
         * @return the reference to a cached array into which the result values will
         *         be put. The result[0] will contain frequency for 1-gram etc.
         * @throws Exception in case this function is used when query result caching is not done.
         */
        virtual SFrequencyResult<N> & queryWordFreqs(const string & word ) throw (Exception) = 0;

        /**
         * Allows to force reset of internal query caches, if they exist
         */
        virtual void resetQueryCache() = 0;
    };
    
    //Handy type definitions for the tries of different sizes and with.without caches
    typedef ATrie<N_GRAM_PARAM,true> TFiveCacheTrie;
    typedef ATrie<N_GRAM_PARAM,false> TFiveNoCacheTrie;
    
    //Make sure that there will be templates instantiated, at least for the given parameter values
    template class ATrie<N_GRAM_PARAM,true>;
    template class ATrie<N_GRAM_PARAM,false>;
}
#endif	/* ITRIES_HPP */

