/* 
 * File:   trans_job_request.cpp
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
 * Created on June 16, 2016, 14:27 PM
 */

#include "common/messaging/trans_job_request.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                        const string trans_job_request::JOB_ID_NAME = "job_id";
                        const string trans_job_request::SOURCE_LANG_NAME = "source_lang";
                        const string trans_job_request::TARGET_LANG_NAME = "target_lang";
                        const string trans_job_request::TRANS_INFO_FLAG_NAME = "trans_info";
                        const string trans_job_request::SOURCE_SENTENCES_NAME = "source_sent";
                }
            }
        }
    }
}