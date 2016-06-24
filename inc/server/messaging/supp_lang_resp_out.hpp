/* 
 * File:   supp_lang_resp_out.hpp
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
 * Created on June 23, 2016, 3:37 PM
 */

#ifndef SUPP_LANG_RESP_OUT_HPP
#define SUPP_LANG_RESP_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/supp_lang_resp.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    class supp_lang_resp_out : public supp_lang_resp, public outgoing_msg {
                    public:

                        /**
                         * The basic class constructor
                         */
                        supp_lang_resp_out()
                        : supp_lang_resp(), outgoing_msg(msg_type::MESSAGE_SUPP_LANG_RESP) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~supp_lang_resp_out() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to add a pair of supported source-target languages
                         * @param source the source language that can be translated into the target language
                         * @param target the target language that can be translated in to the source language
                         */
                        inline void add_supp_lang(const string& source, const string& target) {
                            get_json()[LANGUAGES_FIELD_NAME][source].push_back(target);
                        }
                    };
                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESP_OUT_HPP */

