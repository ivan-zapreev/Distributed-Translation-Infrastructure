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
#define HASHINGWORDINDEX_HPP

#include <string>   // std::string

#include "server/lm/lm_consts.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "aword_index.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::utils::exceptions;
using namespace uva::utils::hashing;
using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace dictionary {

                        /**
                         * This is a hashing word index, it is trivial - each word gets an id which is its hash value.
                         * This also means that any word is considered to be a known word. Therefore, in the Tries if
                         * the word id has no associated payload then an unknown word payload is to be used.
                         * Still the unknown and undefined word ids are reserved nd should not be issued.
                         */
                        class hashing_word_index : public aword_index {
                        public:

                            /**
                             * The basic constructor
                             * @param memory_factor is not used, is here only for interface compliancy
                             */
                            hashing_word_index(const float memory_factor) : aword_index() {
                                ASSERT_CONDITION_THROW((sizeof (uint64_t) != sizeof (word_uid)),
                                        string("Currently only works with 64 bit word_uid!"));
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
                                return num_words + EXTRA_NUMBER_OF_WORD_IDs;
                            };

                            /**
                             * Does not detect unknown words.
                             * The returned word id is >= MIN_KNOWN_WORD_ID
                             * @see AWordIndex
                             */
                            inline word_uid get_word_id(const text_piece_reader & token) const {
                                //Return the word index making sure that it is at least
                                //equal to two. So that the undefined and unknown word
                                //indexes are not used and no overflow or other checks.
                                const uint64_t hash_value = compute_hash(token.get_begin_c_str(), token.length());
                                const word_uid word_id = hash_value | (1 << 1);
                                LOG_DEBUG2 << "Hashing '" << token << "' into: " << hash_value << ", resulting id is: " << word_id << END_LOG;
                                return word_id;
                            };

                            /**
                             * The returned word id is >= MIN_KNOWN_WORD_ID
                             * @see AWordIndex
                             */
                            inline bool is_word_registering_needed() const {
                                return false;
                            };

                            /**
                             * The word registration is not needed, for this word index.
                             * @see AWordIndex
                             */
                            inline word_uid register_word(const text_piece_reader & token) {
                                THROW_MUST_NOT_CALL();
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
                            virtual ~hashing_word_index() {
                            };
                        };
                    }
                }
            }
        }
    }
}

#endif /* HASHINGWORDINDEX_HPP */

