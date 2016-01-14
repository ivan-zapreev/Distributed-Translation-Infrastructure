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

#include "components/lm/TrieConstants.hpp"
#include "components/lm/TrieConfigs.hpp"
#include "components/logging/Logger.hpp"
#include "utils/file/TextPieceReader.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::smt::tries::identifiers;

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
                 * 
                 * NOTE: All of the methods are non-virtual for the sake of avoiding virtual method call overheads!
                 */
                template<typename TIdType>
                class AWordIndex {
                public:
                    //The issued id type
                    typedef TIdType TWordIdType;

                    //Stores the string representation of an unknown word
                    const static string UNKNOWN_WORD_STR;

                    //Stores the word hash for an unknown word, is 0
                    //WARNING! MUST BE 0 as this is the value of a default initialized integer!
                    const static TIdType UNDEFINED_WORD_ID;

                    //Stores the word id for an unknown word, it must have value 1
                    const static TIdType UNKNOWN_WORD_ID;

                    //Stores the minimum known word id, it must have value 2
                    const static TIdType MIN_KNOWN_WORD_ID;

                    //The word indexes that start from 2, as 0 is given to UNDEFINED and 1 to UNKNOWN (<unk>)
                    const static TIdType EXTRA_NUMBER_OF_WORD_IDs;

                    /**
                     * This method should be used to pre-allocate the word index
                     * @param num_words the number of words
                     */
                    inline void reserve(const size_t num_words) {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * Allows to get the total words count including the unknown and undefined words
                     * @param num_words the number of words in the language model
                     */
                    inline size_t get_number_of_words(const size_t num_words) const {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * This function gets an id for the given word word based no the stored 1-Grams.
                     * Continuous word index: If the word is not known then an unknown word ID is
                     *                        returned: UNKNOWN_WORD_ID
                     * Discontinuous word index: The returned word id is always >= MIN_KNOWN_WORD_ID
                     * @param token the word to hash
                     * @return the word id or UNKNOWN_WORD_ID if the word is not found
                     */
                    inline TIdType get_word_id(const TextPieceReader & token) const {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * This method allows to indicate whether registering a word is
                     * needed by the given implementation of the word index.
                     * @return true if the word registering is needed, otherwise false.
                     */
                    inline bool is_word_registering_needed() const {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * This function creates/gets an id for the given word.
                     * Note: The ids must be unique and continuous!
                     * The returned word id is always >= MIN_KNOWN_WORD_ID
                     * @param token the word to hash
                     * @return the resulting hash
                     */
                    inline TIdType register_word(const TextPieceReader & token) {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * This method allows to indicate whether word counting is
                     * needed by the given implementation of the word index.
                     * @return true if the word counting is needed, otherwise false.
                     */
                    inline bool is_word_counts_needed() const {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * This method is to be used when the word counting is needed.
                     * The main application here is to first count the number of
                     * word usages and then distribute the word ids in such a way
                     * that the most used words get the lowest ids.
                     * @param token the word to count
                     * @param prob the word log probability
                     */
                    inline void count_word(const TextPieceReader & word, TLogProbBackOff prob) {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * Should be called if the word count is needed
                     * after all the words have been counted.
                     */
                    inline void do_post_word_count() {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * Indicates if the post-actions are needed. The post actions
                     * should be called after all the words have been filled into
                     * the index.
                     * @return true if the post-actions are needed, otherwise false
                     */
                    inline bool is_post_actions_needed() const {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * Is to be called if the post actions are needed right after
                     * that all the individual words have beed added into the index.
                     */
                    inline void do_post_actions() {
                        THROW_MUST_OVERRIDE();
                    };

                    /**
                     * Allows to indicate if the word index is continuous, i.e.
                     * it issues the word ids in a continuous range starting from 0.
                     * Where 0 and 1 are reserved word ids.
                     * 
                     * If the word index is not continuous then the uni-gram payload
                     * can not be stored in a word id indexed array. Moreover, any word
                     * id considered to be a known word, i.e. the unknown word id
                     * is never returned by the word index.
                     * 
                     * This method is to be overridden by the children classes.
                     * The default implementation returns false!
                     */
                    static constexpr inline bool is_word_index_continuous() {
                        return false;
                    }

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

