/* 
 * File:   adapters_manager.cpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#include "balancer/translator_adapter.hpp"
#include "balancer/adapters_manager.hpp"
#include "balancer/translation_manager.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                /*Translation server adapter*/
                id_manager<trans_server_uid> translator_adapter::m_ids_manager(0);

                /*Translation manager*/
                const balancer_parameters * translation_manager::m_params = NULL;
                translation_manager::response_sender translation_manager::m_sender_func = NULL;

                /*Translation servers' manager*/
                const balancer_parameters * adapters_manager::m_params = NULL;
                adapters_manager::adapters_map adapters_manager::m_adapters_data;
                thread * adapters_manager::m_re_connect = NULL;
                mutex adapters_manager::m_re_connect_mutex;
                condition_variable adapters_manager::m_re_connect_condition;
                a_bool_flag adapters_manager::m_is_reconnect_run(true);
                mutex adapters_manager::m_source_mutex;
                adapters_manager::sources_map adapters_manager::m_sources;
                supp_lang_resp_out adapters_manager::m_supp_lan_resp;
                string adapters_manager::m_supp_lan_resp_str;
                shared_mutex adapters_manager::m_supp_lang_mutex;
            }
        }
    }
}