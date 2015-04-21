/* 
 * File:   NGramBuilder.hpp
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
 * Created on April 18, 2015, 12:02 PM
 */

#ifndef NGRAMBUILDER_HPP
#define	NGRAMBUILDER_HPP

#include <ATrie.hpp>

#include <string>  //std::string
#include <vector>  //std::vector

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

