/* 
 * File:   supp_lang_response.hpp
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
 * Created on June 16, 2016, 2:38 PM
 */

#ifndef SUPP_LANG_RESPONSE_HPP
#define SUPP_LANG_RESPONSE_HPP

#include <string>
#include <map>
#include <vector>

using namespace std;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This class represents the supported languages response message
                     */
                    class supp_lang_response {
                    public:
                        //The begin of the supported languages request message
                        static const string SUPP_LANG_RESPONSE_PREFIX;

                        //Define the map used to store the source-target languages
                        typedef map<string, vector<string>> source_2_targets_map;

                        /**
                         * The basic constructor
                         */
                        supp_lang_response() : m_source_2_target() {
                        }

                        /**
                         * Allows to add a pair of supported source-target languages
                         * @param source the source language that can be translated into the target language
                         * @param target the target language that can be translated in to the source language
                         */
                        inline void add_supp_lang(const string& source, const string& target) {
                            m_source_2_target[source].push_back(target);
                        }

                        /**
                         * Allows to get the response's source-to-target language mapping
                         * @return the source-to-target language mapping
                         */
                        inline const source_2_targets_map get_supp_langs() {
                            return m_source_2_target;
                        }

                        /**
                         * Allows to serialize the supported languages response into a string
                         * @return the string representation of the supported languages response
                         */
                        inline const string serialize() const {
                            string result = SUPP_LANG_RESPONSE_PREFIX;

                            //Serialize the source to targets mappings in the JSON format
                            for (auto iter_map = m_source_2_target.begin(); iter_map != m_source_2_target.end(); ++iter_map) {
                                result += "{ \"" + iter_map->first + "\" : [ ";
                                for (auto iter_arr = iter_map->second.begin(); iter_arr != iter_map->second.end(); ++iter_arr) {
                                    result += "\"" + *iter_arr + "\", ";
                                }
                                //Remove the last ", " from the result and append the closing brackets
                                result = result.substr(0, result.length() - 2) + " ] }";
                            }

                            return result;
                        }

                    private:
                        //Stores the source to target language pairs mappings
                        source_2_targets_map m_source_2_target;
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESPONSE_HPP */

