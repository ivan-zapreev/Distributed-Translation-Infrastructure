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

                    class supp_lang_resp_out : public outgoing_msg, public supp_lang_resp {
                    public:

                        /**
                         * The basic class constructor
                         */
                        supp_lang_resp_out()
                        : outgoing_msg(msg_type::MESSAGE_SUPP_LANG_RESP), supp_lang_resp() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic class destructor
                         */
                        virtual ~supp_lang_resp_out() {
                            //Nothing to be done here
                        }

                        /**
                         * Allows to start the supported languages object entry
                         */
                        inline void start_supp_lang_obj() {
                            m_writer.String(LANGUAGES_FIELD_NAME);
                            m_writer.StartObject();
                        }

                        /**
                         * Allows to end the supported languages object entry
                         */
                        inline void end_supp_lang_obj() {
                            m_writer.EndObject();
                        }

                        /**
                         * Allows to start the source language entry array
                         * @param source the source language that can be translated into the target language
                         */
                        inline void start_source_lang_arr(const string& source) {
                            m_writer.String(source.c_str());
                            m_writer.StartArray();
                        }

                        /**
                         * Allows to end the source language entry array
                         */
                        inline void end_source_lang_arr() {
                            m_writer.EndArray();
                        }

                        /**
                         * Allows to add a target language
                         * @param target the target language that can be translated in to the source language
                         */
                        inline void add_target_lang(const string& target) {
                            m_writer.String(target.c_str());
                        }
                    };
                }
            }
        }
    }
}

#endif /* SUPP_LANG_RESP_OUT_HPP */

