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

                /**
                 * This abstract class is used to represent the word dictionary.
                 * It contains no specific implementation but is more of an interface.
                 * It is used to allow for more word dictionary/index implementations.
                 * 
                 * Any implementation of this class must issue the unknown word
                 * <unk> index 1 (UNKNOWN_WORD_ID).
                 * 
                 * The first real word index will be therefore 2 (MIN_KNOWN_WORD_ID).
                 * 
                 * The issued word ids must be continuous and non-repeating, unique!
                 */
                class AWordIndex {
                public:

                    //Stores the string representation of an unknown word
                    const static string UNKNOWN_WORD_STR;

                    //Stores the word hash for an unknown word, is 0
                    //WARNING! MUST BE 0 as this is the value of a default initialized integer!
                    const static TShortId UNDEFINED_WORD_ID;

                    //Stores the word id for an unknown word, it must have value 1
                    const static TShortId UNKNOWN_WORD_ID;

                    //Stores the minimum known word id, it must have value 2
                    const static TShortId MIN_KNOWN_WORD_ID;

                    //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                    const static TShortId EXTRA_NUMBER_OF_WORD_IDs;

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    virtual void reserve(const size_t num_words) = 0;

                    /**
                     * Allows to get the total words count including the unknown and undefined words
                     * @param num_words the number of words in the language model
                     */
                    virtual size_t get_number_of_words(const size_t num_words) = 0;
                    
                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_ID
                     * @param token the word to hash
                     * @param wordId the resulting wordId or UNKNOWN_WORD_ID if the word is not found
                     * @return true if the word id is found, otherwise false
                     */
                    virtual bool get_word_id(const string & token, TShortId &wordId) const = 0;

                    /**
                     * This method is to be used when the word counting is needed.
                     * The main application here is to first count the number of
                     * word usages and then distribute the word ids in such a way
                     * that the most used words get the lowest ids.
                     * If not re-implemented throws an exception.
                     * @param token the word to count
                     */
                    virtual void count_word(const TextPieceReader & token) {};
                    
                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * The default implementation always returns false.
                     * @return true if the word counting is needed, otherwise false.
                     */
                    virtual bool need_word_counts(){return false;};
                    
                    /**
                     * Should be called if the word count is needed
                     * after all the words have been counted.
                     * If not re-implemented throws an exception.
                     */
                    virtual void post_word_count() {};
                    
                    /**
                     * This function creates/gets an id for the given word.
                     * Note: The ids must be unique and continuous!
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    virtual TShortId register_word(const TextPieceReader & token) = 0;

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

