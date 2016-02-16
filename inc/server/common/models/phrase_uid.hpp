/* 
 * File:   tm_phrase_id.hpp
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
 * Created on February 9, 2016, 5:35 PM
 */

#ifndef TM_PHRASE_ID_HPP
#define TM_PHRASE_ID_HPP

#include <string>
#include <vector>
#include <cstdint>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/string_utils.hpp"

#include "server/lm/proxy/lm_index_query_proxy.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::hashing;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::lm::proxy;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace common {
                    namespace models {
                        //Declare the phrase unique identifier type
                        typedef uint64_t phrase_uid;

                        //Define the undefined phrase id value
                        static constexpr uint64_t UNDEFINED_PHRASE_ID = 0;
                        //Define the unknown phrase id value
                        static constexpr uint64_t UNKNOWN_PHRASE_ID = UNDEFINED_PHRASE_ID + 1;
                        //Contains the minimum valid phrase id value
                        static constexpr uint64_t MIN_VALID_PHRASE_ID = UNKNOWN_PHRASE_ID + 1;
                        //Contains the maximum valid phrase id value
                        static constexpr uint64_t MAX_VALID_PHRASE_ID = UINT64_MAX;

                        /**
                         * Allows to get the phrase uid for the given phrase pair.
                         * The current implementation uses the hash function to compute the uid.
                         * Before the computation of the phrase id the phrase strings are NOT trimmed.
                         * @param p1_uid the first phrase uid
                         * @param p2_uid the second phrase uid
                         * @return the uid of the phrase
                         */
                        static inline phrase_uid combine_phrase_uids(const phrase_uid p1_uid, const phrase_uid p2_uid) {
                            //ASSERT that the ids are not undefined
                            ASSERT_SANITY_THROW((p1_uid == UNDEFINED_PHRASE_ID) || (p2_uid == UNDEFINED_PHRASE_ID),
                                    string("Trying to combine two ids where at least one id undefined, first uid: ") +
                                    to_string(p1_uid) + string(", second uid: ") + to_string(p2_uid));

                            //Compute the combined uid
                            phrase_uid uid = combine_hash(p2_uid, p1_uid);

                            //If the value is below the minimum then shift it up
                            if (uid < MIN_VALID_PHRASE_ID) {
                                uid += MIN_VALID_PHRASE_ID;
                            }
                            return uid;
                        }

                        /**
                         * Allows to get the source phrase uid for the given phrase.
                         * Note: The current implementation uses the hash function to compute the uid.
                         * Note: The phrase string is taken as is.
                         * @parm is_token if true then the given phrase is treated as one token
                         *       if false then it is space separated list of tokens, in the latter
                         *       case the uid will be computed in a cumulative fashion. Default is false.
                         * @param the phrase to get the uid for
                         * @return the uid of the phrase
                         */
                        template<bool is_token = false >
                        static inline phrase_uid get_source_phrase_uid(const string & phrase) {
                            //Declare and default initialize the uid
                            phrase_uid uid = UNDEFINED_PHRASE_ID;

                            //Check if we shall threat the given phrase as a single token or not.
                            if (is_token) {
                                //If it is just a single token then compute the string hash
                                uid = compute_hash(phrase);
                            } else {
                                //This phrase is to be treated as a phrase
                                const string& delim = UTF8_SPACE_STRING;
                                //Declare and initialie the token's begin and end char index
                                size_t start = 0, end = phrase.find_first_of(delim);
                                //Get the uid of the first token
                                uid = compute_hash(phrase.substr(start, end - start));

                                //If there is more in the string to process search for
                                //the next delimiter(s) and compute cumulative ids
                                if (end != std::string::npos) {
                                    start = end + 1;
                                    end = phrase.find_first_of(delim, start);
                                    while (end <= std::string::npos) {
                                        uid = combine_phrase_uids(uid, compute_hash(phrase.substr(start, end - start)));
                                        if (end != std::string::npos) {
                                            start = end + 1;
                                            end = phrase.find_first_of(delim, start);
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            }

                            //If the value is below the minimum then shift it up
                            if (uid < MIN_VALID_PHRASE_ID) {
                                uid += MIN_VALID_PHRASE_ID;
                            }
                            return uid;
                        }

                        /**
                         * Allows to get the target phrase uid for the given phrase.
                         * Note: The current implementation uses the hash function to compute the uid.
                         * Note: The phrase string is taken as is.
                         * ToDo: Use the word index of the language model in order to compute the uid.
                         * @parm is_token if true then the given phrase is treated as one token
                         *       if false then it is space separated list of tokens, in the latter
                         *       case the uid will be computed in a cumulative fashion. Default is false.
                         * @param phrase the target phrase to get the uid for
                         * @return the uid of the target phrase
                         */
                        template<bool is_token = false, bool is_word_index = true >
                        static inline phrase_uid get_target_phrase_uid(const string & phrase, lm_index_query_proxy * word_index = NULL) {
                            ASSERT_SANITY_THROW(is_word_index && (word_index == NULL),
                                    string("Requesting a target phrase id using the word ") +
                                    string("index but the word index pointer is NULL!"));

                            //ToDo: Implement using the word index of the language model to get the single token uid
                            return get_source_phrase_uid<is_token>(phrase);
                        }
                    }
                }
            }
        }
    }
}

#endif /* TM_PHRASE_ID_HPP */

