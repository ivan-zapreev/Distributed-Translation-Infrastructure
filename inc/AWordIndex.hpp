/* 
 * File:   AWordIndex.hpp
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
 * Created on August 21, 2015, 12:28 PM
 */

#ifndef AWORDINDEX_HPP
#define	AWORDINDEX_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::hashing;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                //Stores the string representation of an unknown word
                const string UNKNOWN_WORD_STR = "<unk>";

                /**
                 * This abstract class is used to represent the word dictionary.
                 * It contains no specific implementation but is more of an interface.
                 * It is used to allow for more word dictionary/index implementations.
                 */
                class AWordIndex {
                public:

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void preAllocate(const size_t num_words) = 0;

                    /**
                     * This function gets a hash for the given word word based no the stored 1-Grams.
                     * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_HASH
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    virtual TWordHashSize getUniqueIdHash(const string & token) const = 0;

                    /**
                     * This function creates/gets a hash for the given word.
                     * Note: The hash id will be unique and continuous!
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    virtual TWordHashSize createUniqueIdHash(const TextPieceReader & token) = 0;

                    /**
                     * The basic destructor
                     */
                    virtual ~AWordIndex() {
                    };

                };
            }
        }
    }
}

#endif	/* WORDINDEX_HPP */

