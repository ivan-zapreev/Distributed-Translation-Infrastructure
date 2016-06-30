/* 
 * File:   messaging.cpp
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
 * Created on June 23, 2016, 3:30 PM
 */

#include "common/messaging/msg_base.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/request_msg.hpp"
#include "common/messaging/response_msg.hpp"
#include "common/messaging/supp_lang_req.hpp"
#include "common/messaging/supp_lang_resp.hpp"
#include "common/messaging/trans_job_req.hpp"
#include "common/messaging/trans_job_resp.hpp"
#include "common/messaging/trans_sent_data.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    
                    constexpr uint32_t msg_base::PROTOCOL_VERSION;
                    const char * msg_base::PROT_VER_FIELD_NAME = "prot_ver";
                    const char * msg_base::MSG_TYPE_FIELD_NAME = "msg_type";

                    const char * response_msg::STAT_CODE_FIELD_NAME = "stat_code";
                    const char * response_msg::STAT_MSG_FIELD_NAME = "stat_msg";

                    const char * trans_job_req::JOB_ID_FIELD_NAME = "job_id";
                    const char * trans_job_req::SOURCE_LANG_FIELD_NAME = "source_lang";
                    const char * trans_job_req::TARGET_LANG_FIELD_NAME = "target_lang";
                    const char * trans_job_req::IS_TRANS_INFO_FIELD_NAME = "is_trans_info";
                    const char * trans_job_req::SOURCE_SENTENCES_FIELD_NAME = "source_sent";

                    const char * supp_lang_resp::LANGUAGES_FIELD_NAME = "langs";
                    
                    const char * trans_job_resp::JOB_ID_FIELD_NAME = "job_id";
                    const char * trans_job_resp::TARGET_DATA_FIELD_NAME = "target_data";

                    const char * trans_sent_data::TRANS_TEXT_FIELD_NAME = "trans_text";
                    const char * trans_sent_data::STACK_LOAD_FIELD_NAME = "stack_load";
                }
            }
        }
    }
}