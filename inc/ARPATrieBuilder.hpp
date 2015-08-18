/* 
 * File:   TrieBuilder.hpp
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
 * Created on April 18, 2015, 11:58 AM
 */

#ifndef TRIEBUILDER_HPP
#define	TRIEBUILDER_HPP

#include <fstream>      // std::ifstream
#include <regex>        // std::regex, std::regex_match

#include "ATrie.hpp"
#include "MemoryMappedFileReader.hpp"

using namespace std;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                /**
                 * This is the Trie builder class that reads an input file stream
                 * and creates n-grams and then records them into the provided Trie.
                 */
                template<TModelLevel N>
                class ARPATrieBuilder {
                public:
                    /**
                     * The basic constructor that accepts a trie to be build up and the file stream to read from
                     * @param trie the trie to fill in with data from the text corpus
                     * @param _fstr the file stream to read from
                     */
                    ARPATrieBuilder(ATrie<N> & trie, MemoryMappedFileReader & _fstr);

                    /**
                     * This function will read from the file and build the trie
                     */
                    void build();

                    virtual ~ARPATrieBuilder();
                private:
                    //The reference to the trie to be build
                    ATrie<N> & _trie;
                    //The reference to the input file with text corpus
                    MemoryMappedFileReader & _fstr;
                    //Stores the next line data
                    BasicTextFileReader _line;
                    //The regular expression for matching the n-gram amount entry of the data section
                    const regex _ngAmountRegExp;
                    //The regular expression for matching the n-grams section
                    const regex _ngSectionRegExp;

                    /**
                     * The copy constructor
                     * @param orig the other builder to copy
                     */
                    ARPATrieBuilder(const ARPATrieBuilder& orig);

                    /**
                     * This method is used to read and process the ARPA headers
                     * @param line the in/out parameter storing the last read line
                     */
                    void readHeaders();

                    /**
                     * This method is used to read and process the ARPA data section
                     * @param line the in/out parameter storing the last read line
                     * @param counts the out parameters to store the retrieved
                     *               N-Gram counts from the data descriptions
                     */
                    void readData(size_t counts[N]);

                    /**
                     * This recursive method is used to read and process the ARPA N-Grams.
                     * @param line the in/out parameter storing the last read line
                     * @param level the level we are to read
                     */
                    void readNGrams(const TModelLevel level);
                };
            }
        }
    }
}
#endif	/* TRIEBUILDER_HPP */

