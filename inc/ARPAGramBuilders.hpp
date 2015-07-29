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

#include <regex>      // std::regex, std::regex_match
#include <functional> // std::function 

#include <ATrie.hpp>

#include "Globals.hpp"
#include <Exceptions.hpp>

using namespace std;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                typedef std::function<void (const SBackOffNGram&)> TAddGramFunct;
                
                /**
                 * This class is responsible for splitting a piece of text in a number of ngrams and place it into the trie
                 */
                class ARPAGramBuilder {
                public:

                    /**
                     * The constructor to be used in order to instantiate a N-Gram builder
                     * @param level the level of the N-grams to be processed
                     * @param trie the trie to be filled in with the N-gram
                     * @param delim the delimiter for the N-gram string
                     */
                    ARPAGramBuilder(const TModelLevel level, TAddGramFunct addGarmFunc, const char delim);

                    /**
                     * This pure virtual method is supposed to parse the N-Gram
                     * string from the ARPA file format of a Back-Off language
                     * model and then add the obtained data to the Trie.
                     * This method has a default implementation that should work
                     * for N-grams with level > MIN_NGRAM_LEVEL and level < N
                     * @param data the string to process, has to be space a
                     *             separated sequence of tokens
                     * @result returns true if the provided line is NOT recognized
                     *         as the N-Gram of the specified level.
                     */
                    bool processString(const string & data);

                    virtual ~ARPAGramBuilder();
                protected:
                    //The function that is to be used to add an N-gram to a trie
                    TAddGramFunct _addGarmFunc;
                    //The tokens delimiter in the string to parse
                    const char _delim;
                    //The level of the N-grams to be processed by the given builder
                    const TModelLevel _level;
                    //This is the N-Gram container to store the parsed N-gram data
                    SBackOffNGram _ngram;
                    //The minimum and maximum number of tokens in the N-Gram string
                    const int MIN_NUM_TOKENS_NGRAM_STR;
                    const int MAX_NUM_TOKENS_NGRAM_STR;
                    
                    /**
                     * The copy constructor
                     * @param orig the other builder to copy
                     */
                    ARPAGramBuilder(const ARPAGramBuilder& orig);

                };
            }
        }
    }
}
#endif	/* NGRAMBUILDER_HPP */

