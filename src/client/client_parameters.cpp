/* 
 * File:   client_parameters.cpp
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
 * Created on May 29, 2018, 10330 AM
 */

#include "client/client_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                const string client_parameters::CL_DEF_TLS_MODE = tls_val_to_str(tls_mode_enum::MOZILLA_UNDEFINED);
                const string client_parameters::CL_DEF_TLS_CIPHERS = "";
                
                const string client_parameters::CL_DEF_PRE_PROC_URI = "";
                const string client_parameters::CL_DEF_TRANS_URI = "ws://localhost:9002";
                const string client_parameters::CL_DEF_POST_PROC_URI = "";
                
                const uint32_t client_parameters::CL_DEF_MIN_SENT_VAL = 100;
                const uint32_t client_parameters::CL_DEF_MAX_SENT_VAL = 100;
                const int32_t client_parameters::CL_DEF_PRIORITY_VAL = 0;
                const bool client_parameters::CL_DEF_IS_TRANS_INFO_VAL = false;
                
                const string client_parameters::CL_CONFIG_SECTION_NAME = "Client Options";
                const string client_parameters::CL_PRE_PARAMS_SECTION_NAME = "Pre-processor Options";
                const string client_parameters::CL_TRANS_PARAMS_SECTION_NAME = "Translator Options";
                const string client_parameters::CL_POST_PARAMS_SECTION_NAME = "Post-processor Options";

                const string client_parameters::CL_MIN_SENT_PARAM_NAME = "min_sent_count";
                const string client_parameters::CL_MAX_SENT_PARAM_NAME = "max_sent_count";
                const string client_parameters::CL_PRIORITY_PARAM_NAME = "job_priority";
                const string client_parameters::CL_IS_TRANS_INFO_PARAM_NAME = "is_trans_info";
            }
        }
    }
}