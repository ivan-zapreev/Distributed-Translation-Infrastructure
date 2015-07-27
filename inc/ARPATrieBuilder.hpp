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

#include "ATrie.hpp"

using namespace std;

namespace uva {
    namespace smt {
        namespace tries {

            /**
             * This is the Trie builder class that reads an input file stream
             * and creates n-grams and then records them into the provided Trie.
             */
            template<TTrieSize N, bool doCache>
            class ARPATrieBuilder {
            public:
                /**
                 * The basic constructor that accepts a trie to be build up and the file stream to read from
                 * @param trie the trie to fill in with data from the text corpus
                 * @param _fstr the file stream to read from
                 * @param delim the delimiter for the line elements
                 */
                ARPATrieBuilder(ATrie<N, doCache> & trie, ifstream & _fstr, const char delim);

                /**
                 * This function will read from the file and build the trie
                 */
                void build();

                virtual ~ARPATrieBuilder();
            private:
                //The reference to the trie to be build
                ATrie<N, doCache> & _trie;
                //The reference to the input file with text corpus
                ifstream & _fstr;
                //The delimiter for the line elements
                const char _delim;

                /**
                 * The copy constructor
                 * @param orig the other builder to copy
                 */
                ARPATrieBuilder(const ARPATrieBuilder& orig);
            };
        }
    }
}
#endif	/* TRIEBUILDER_HPP */

