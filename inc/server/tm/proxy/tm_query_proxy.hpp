/* 
 * File:   tm_query_proxy.hpp
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
 * Created on February 8, 2016, 9:58 AM
 */

#ifndef TM_QUERY_PROXY_HPP
#define TM_QUERY_PROXY_HPP

#include "server/tm/tm_configs.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace proxy {

                        /**
                         * This class represents a translation query proxy interface class.
                         * It allows to interact with translation model queries in a uniform way.
                         */
                        class tm_query_proxy {
                        public:
                            /**
                             * Allows to add the source phrase to the query.
                             * @param uid the source phrase uid
                             * @param entry_ptr the reference to the source entry pointer to be set with the data
                             */
                            virtual void execute(const phrase_uid uid, tm_const_source_entry_ptr & entry_ptr) = 0;

                            /**
                             * Allows retrieve the UNK phrase entry
                             * @return the pointer to the UNK entry
                             */
                            virtual tm_const_source_entry * get_unk_entry() = 0;

                            /**
                             * Allows retrieve the translations of the given source entry
                             * @param uid the unique identifier of the source phrase
                             * @return the pointer to the source entry or NULL if the translation is not found
                             */
                            virtual tm_const_source_entry * get_source_entry(const phrase_uid uid) = 0;

                            /**
                             * Allows to get all the source/target phrase identifiers
                             * for the source target translation in this query.
                             * Must be called after the query is executed
                             * @param st_uids the container for the source/target phrase identifiers
                             */
                            virtual void get_st_uids(vector<phrase_uid> & st_uids) const = 0;

                            /**
                             * Allows to retrieve the unknown source word log probability penalty 
                             * @return the unknown source word log probability penalty
                             */
                            inline prob_weight get_unk_word_prob() const {
                                return tm::UNK_SOURCE_WORD_LOG_PROB;
                            }

                            /**
                             * The basic virtual destructor
                             */
                            virtual ~tm_query_proxy() {
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_QUERY_PROXY_HPP */

