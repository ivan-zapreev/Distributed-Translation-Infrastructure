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

#include "common/messaging/json_msg.hpp"

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
                        
                        //Stores the languages field name for the JSON message
                        static const string LANGUAGES_FIELD_NAME;

                        /**
                         * The basic constructor
                         */
                        supp_lang_response() : m_json_obj(msg_type::MESSAGE_SUPP_LANG_RESP) {
                        }

                        /**
                         * Allows to add a pair of supported source-target languages
                         * @param source the source language that can be translated into the target language
                         * @param target the target language that can be translated in to the source language
                         */
                        inline void add_supp_lang(const string& source, const string& target) {
                            m_json_obj.m_json_obj[LANGUAGES_FIELD_NAME][source].push_back(target);
                        }

                        /**
                         * Allows to serialize the supported languages response into a string
                         * @return the string representation of the supported languages response
                         */
                        inline const string serialize() const {
                            return m_json_obj.serialize();
                        }

                    private:
                        //Stores the source to target language pairs mappings
                        json_msg m_json_obj;
                    };

                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESPONSE_HPP */

