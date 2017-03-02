/* 
 * File:   trans_sent_data_out.hpp
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

#ifndef TRANS_SENT_DATA_OUT_HPP
#define TRANS_SENT_DATA_OUT_HPP

#include "common/utils/logging/logger.hpp"

#include "common/messaging/trans_sent_data.hpp"
#include "common/messaging/status_code.hpp"
#include "common/messaging/outgoing_msg.hpp"

#include "common/messaging/trans_sent_data.hpp"

using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace messaging {

                    /**
                     * This class represents the translated sentence data object to be sent out to the client.
                     */
                    class trans_sent_data_out : public trans_sent_data {
                    public:

                        /**
                         * The basic constructor. This class is just a wrapper for a
                         * JSON object, but it does not own it.
                         * @param writer the reference to the encapsulated JSON object
                         */
                        trans_sent_data_out(JSONWriter & writer)
                        : trans_sent_data(), m_writer(writer) {
                        }

                        /**
                         * Begin the sentence data object entry
                         */
                        inline void begin_sent_data_ent() {
                            m_writer.StartObject();
                        }

                        /**
                         * End the sentence data object entry
                         */
                        inline void end_sent_data_ent() {
                            m_writer.EndObject();
                        }

                        /**
                         * Allows to set the message status: code and message
                         * @param code the status code
                         * @param msg the status message
                         */
                        inline void set_status(const status_code & code, const string & msg) {
                            LOG_DEBUG << "Setting the sentence status code: " << to_string(code)
                                    << ", message: " << msg << END_LOG;

                            m_writer.String(STAT_CODE_FIELD_NAME);
                            m_writer.Int(code.val());
                            m_writer.String(STAT_MSG_FIELD_NAME);
                            m_writer.String(msg.c_str());
                        }

                        /**
                         * Allows to get the translation text
                         * @param  text the reference to the translation text string
                         */
                        inline void set_trans_text(const string & text) {
                            LOG_DEBUG << "Setting the translated sentence: " << text << END_LOG;

                            m_writer.String(TRANS_TEXT_FIELD_NAME);
                            m_writer.String(text.c_str());
                        }

                        /**
                         * Allows to start the loads array section in the sentence data
                         */
                        inline void start_loads_arr() {
                            m_writer.String(STACK_LOAD_FIELD_NAME);
                            m_writer.StartArray();
                        }

                        /**
                         * Allows to end the loads array section in the sentence data
                         */
                        inline void end_loads_arr() {
                            m_writer.EndArray();
                        }

                        /**
                         * Allows to add the translation stack load
                         * @param load the next stack load value
                         */
                        inline void add_stack_load(const float load) {
                            LOG_DEBUG1 << "Adding the stack load: " << to_string(load) << END_LOG;
                            m_writer.Uint(load);
                        }

                    private:
                        //Stores a non NULL pointer to the encapsulated JSON object
                        JSONWriter & m_writer;
                    };

                }
            }
        }
    }
}

#endif /* TRANS_SENT_DATA_OUT_HPP */

