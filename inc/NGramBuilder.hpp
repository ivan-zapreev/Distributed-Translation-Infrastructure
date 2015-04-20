/* 
 * File:   NGramBuilder.hpp
 * Author: zapreevis
 *
 * Created on April 18, 2015, 12:02 PM
 */

#ifndef NGRAMBUILDER_HPP
#define	NGRAMBUILDER_HPP

#include <ATrie.hpp>
#include <string>
#include <vector>

using namespace std;

namespace tries {
namespace ngrams {
    /**
     * This class is responsible for splitting a piece of text in a number of ngrams and place it into the trie
     */
    template<TTrieSize N, bool doCache>
    class NGramBuilder {
    public:
        NGramBuilder(ATrie<N,doCache> & trie, const char delim);

        /**
         * For the given text will split it into the number of n-grams that will be then put into the trie
         * @param data the string to process, has to be space a separated sequence of tokens
         */
        void processString(const string & data );

        virtual ~NGramBuilder();
    private:
        //The trie to store the n-grams 
        ATrie<N,doCache> & _trie;
        //The tokens delimiter in the string to parse
        const char _delim;
        //The internal bugger for storing strings
        vector<string> result;

        /**
         * The copy constructor
         * @param orig the other builder to copy
         */
        NGramBuilder(const NGramBuilder& orig);

        /**
         * Tokenise a given string into avector of strings
         * @param s the string to tokenise
         * @param delim the delimiter
         * @param elems the output array
         */
        void tokenize(const std::string &data, const char delim, vector<string> & elems);
    };
}
}
#endif	/* NGRAMBUILDER_HPP */

