/* 
 * File:   HashingWordIndex.hpp
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
 * Created on November 20, 2015, 1:16 PM
 */

#ifndef HASHINGWORDINDEX_HPP
#define	HASHINGWORDINDEX_HPP

#include <string>   // std::string

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

#include "AWordIndex.hpp"
#include "HashingUtils.hpp"
#include "TextPieceReader.hpp"

using namespace std;
using namespace uva::smt::file;
using namespace uva::smt::exceptions;
using namespace uva::smt::tries;

namespace uva {
    namespace smt {
        namespace tries {
            namespace dictionary {

                /**
                 * This is a hashing word index, it is trivial - each word gets an id which is its hash value
                 */
                class HashingWordIndex : public AWordIndex<uint64_t> {
                public:

                    /**
                     * The basic constructor
                     * @param memory_factor is not used, is here only for interface compliancy
                     */
                    HashingWordIndex(const size_t memory_factor) : AWordIndex<uint64_t>() {
                    }

                    /**
                     * @see AWordIndex
                     */
                    inline void reserve(const size_t num_words) {
                        //does nothing
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline size_t get_number_of_words(const size_t num_words) const {
                        return num_words + AWordIndex::EXTRA_NUMBER_OF_WORD_IDs;
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline TWordIdType get_word_id(const TextPieceReader & token) const {
                        return compute_hash(token);
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline bool is_word_registering_needed() const {
                        return false;
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline TWordIdType register_word(const TextPieceReader & token) {
                        return compute_hash(token);
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline bool is_word_counts_needed() const {
                        return false;
                    };

                    /**
                     * @see AWordIndex
                     */
                    inline bool is_post_actions_needed() const {
                        return false;
                    };

                    /**
                     * @see AWordIndex
                     * @return false - this word index is not continuous.
                     */
                    static constexpr inline bool is_word_index_continuous() {
                        return false;
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~HashingWordIndex() {
                    };
                };
            }
        }
    }
}

#endif	/* HASHINGWORDINDEX_HPP */

