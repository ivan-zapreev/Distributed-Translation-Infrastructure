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
#include <sstream> //std::stringstream

#include "Globals.hpp"
#include <Exceptions.hpp>

using namespace std;

namespace uva {
    namespace smt {
        namespace tries {
            namespace ngrams {

                /**
                 * This class is responsible for splitting a piece of text in a number of ngrams and place it into the trie
                 */
                template<TModelLevel N, bool doCache>
                class NGramBuilder {
                public:
                    NGramBuilder(ATrie<N, doCache> & trie, const char delim);

                    /**
                     * For the given text will split it into the number of n-grams that will be then put into the trie
                     * @param data the string to process, has to be space a separated sequence of tokens
                     */
                    void processString(const string & data);

                    /**
                     * This method build an N-Gram from a string, which is nothing more than
                     * just taking a string and tokenizing it with the given delimiter. In
                     * addition this method will test if the resulting N-gram has exactly
                     * the specified number of elements. Will also clean the ngram vector
                     * before filling it in. The order in which the N-gram elements are stored
                     * are the same in which they are present in the given line.
                     * @param line the line of code to convert into an N-gram
                     * @param n the expected value of N
                     * @param delim the delimiter to parse the string into
                     * @param ngram the output parameter that will be filled in with the N-gram values
                     * @throws Exception in case the resulting N-gram has the number elements other than expected
                     */
                    static inline void buildNGram(const string & line, const TModelLevel n, const char delim, vector<string> & ngram) throw (Exception) {
                        //First clean the vector
                        ngram.clear();
                        //Tokenise the line
                        tokenize(line, delim, ngram);
                        //Check that the number of words in the N-gram is proper
                        if (ngram.size() != n) {
                            stringstream msg;
                            msg << "The line '" << line << "' is not a " << n << "-gram as expected!";
                            throw Exception(msg.str());
                        }
                    }

                    virtual ~NGramBuilder();
                private:
                    //The trie to store the n-grams 
                    ATrie<N, doCache> & _trie;
                    //The tokens delimiter in the string to parse
                    const char _delim;

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
                    static inline void tokenize(const std::string &data, const char delim, vector<string> & elems) {
                        stringstream stream(data);
                        string token;

                        //Read from the string stream
                        while (getline(stream, token, delim)) {
                            elems.push_back(token);
                        }
                    }
                };
            }
        }
    }
}
#endif	/* NGRAMBUILDER_HPP */

