/* 
 * File:   TrieBuilder.hpp
 * Author: zapreevis
 *
 * Created on April 18, 2015, 11:58 AM
 */

#ifndef TRIEBUILDER_HPP
#define	TRIEBUILDER_HPP

#include <fstream>      // std::ifstream

#include "ITrie.hpp"
using namespace std;

/**
 * This is the Trie builder class that reads an input file stream
 * and creates n-grams and then records them into the provided Trie.
 */
class TrieBuilder {
public:
    /**
     * The basic constructor that accepts a trie to be build up and the file stream to read from
     * @param trie the trie to fill in with data from the text corpus
     * @param _fstr the file stream to read from
     * @param delim the delimiter for the line elements
     */
    TrieBuilder(ITrie & trie, ifstream & _fstr, const char delim);

    /**
     * This function will read from the file and build the trie
     */
    void build();
    
    virtual ~TrieBuilder();
private:
    //The reference to the trie to be build
    ITrie & _trie;
    //The reference to the input file with text corpus
    ifstream & _fstr;
    //The delimiter for the line elements
    const char _delim;
    
    /**
     * The copy constructor
     * @param orig the other builder to copy
     */
    TrieBuilder(const TrieBuilder& orig);
};

#endif	/* TRIEBUILDER_HPP */

