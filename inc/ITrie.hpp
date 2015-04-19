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

using namespace std;

/**
 * This is a common interface class for all possible Trie implementations
 */
class ITrie {
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
    virtual unsigned int getNGramLevel() = 0;

};

#endif	/* ITRIES_HPP */

