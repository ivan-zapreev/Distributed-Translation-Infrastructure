/* 
 * File:   language_registry.hpp
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
 * Created on July 14, 2016, 1:30 PM
 */

#ifndef LANGUAGE_REGISTRY_HPP
#define LANGUAGE_REGISTRY_HPP

#include <unordered_map>

#include "common/utils/id_manager.hpp"
#include "common/utils/threads/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "balancer/balancer_consts.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::threads;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This singleton class represents a singleton registry for giving
                 * unique ids to the language strings. In this way each language
                 * gets a unique identifier which is to be used within the balancer
                 * in the internal hash maps for fast re-routing messages to the
                 * appropriate translation servers.
                 */
                class language_registry {
                public:
                    //The id value given to an unknown language
                    static constexpr language_uid UNKNONW_LANGUAGE_ID = 0;
                    //The id value given to an unknown language
                    static const string UNKNONW_LANGUAGE_NAME;
                    //The first known language id value
                    static constexpr language_uid MIN_LANGUAGE_ID = UNKNONW_LANGUAGE_ID + 1;

                    /**
                     * Allows to get the language name for its UID
                     * @param uid the language uid
                     * @return the language name, UNKNOWN is returned if the UID is not known.
                     */
                    static inline const string & get_name(const language_uid & uid) {
                        shared_guard read_guard(m_lang_mutex);
                        
                        //Search for the language in the map
                        auto iter = m_id_to_lang.find(uid);

                        //Check if the language is found
                        if (iter != m_id_to_lang.end()) {
                            //Return the language name
                            return iter->second;
                        } else {
                            //Return the unknown language name
                            return UNKNONW_LANGUAGE_NAME;
                        }
                    }

                    /**
                     * A synchronized function that allows to get a unique id for the given language
                     * @param name the language name
                     * @return the language id or UNKNONW_LANGUAGE_ID if the language is not known
                     */
                    static inline language_uid get_uid(const string & name) {
                        shared_guard read_guard(m_lang_mutex);

                        //Search for the language in the map
                        auto iter = m_lang_to_id.find(name);

                        //Check if the language is found
                        if (iter != m_lang_to_id.end()) {
                            //Return the language id
                            return iter->second;
                        } else {
                            //Return the unknown language id
                            return UNKNONW_LANGUAGE_ID;
                        }
                    }

                    /**
                     * Allows to register a new language, if the language is known then no registration is done.
                     * @param name the language name
                     * @return the id issued to the given language
                     */
                    static inline language_uid register_uid(const string & name) {
                        exclusive_guard write_guard(m_lang_mutex);

                        //Get the storage reference
                        language_uid & ref = m_lang_to_id[name];

                        //Check if the value is unknown
                        if (ref == UNKNONW_LANGUAGE_ID) {
                            //The language id is not known, issue a new one!
                            ref = m_id_mgr.get_next_id();
                            //Add the id to language mapping
                            m_id_to_lang[ref] = name;
                        }

                        //Return the id
                        return ref;
                    }

                private:
                    //Stores the id manager for issuing the language ids
                    static id_manager<language_uid> m_id_mgr;
                    //Stores the access mutex for the registry map
                    static shared_mutex m_lang_mutex;
                    //Stores the map to be synchronized which stores mapping from the language name to its id
                    static unordered_map<string, language_uid> m_lang_to_id;
                    //Stores the backwards mapping from the id to its string representation
                    static unordered_map<language_uid, string> m_id_to_lang;
                };
            }
        }
    }
}

#endif /* LANGUAGE_REGISTRY_HPP */

