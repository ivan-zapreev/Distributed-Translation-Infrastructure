/* 
 * File:   AWordIndex.cpp
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
 * Created on September 3, 2015, 16:20 PM
 */

#include <string>   // std::string

#include "AWordIndex.hpp"

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
                const string AWordIndex::UNKNOWN_WORD_STR = "<unk>";

                //Stores the word hash for an unknown word, is 0
                //WARNING! MUST BE 0 as this is the value of a default initialized integer!
                const TShortId AWordIndex::UNDEFINED_WORD_ID = static_cast<TShortId> (0);

                //Stores the word id for an unknown word, it must have value 1
                const TShortId AWordIndex::UNKNOWN_WORD_ID = static_cast<TShortId> (AWordIndex::UNDEFINED_WORD_ID + 1);

                //Stores the minimum known word id, it must have value 2
                const TShortId AWordIndex::MIN_KNOWN_WORD_ID = static_cast<TShortId> (AWordIndex::UNKNOWN_WORD_ID + 1);

                //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                const TShortId AWordIndex::EXTRA_NUMBER_OF_WORD_IDs = 2;
            }
        }
    }
}

