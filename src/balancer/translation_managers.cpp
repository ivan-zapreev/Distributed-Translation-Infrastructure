/* 
 * File:   translation_servers_manager.cpp
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

#include "balancer/translation_server_adapter.hpp"
#include "balancer/translation_servers_manager.hpp"
#include "balancer/translation_manager.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {
                /*Translation server adapter*/
                id_manager<trans_server_uid> translation_server_adapter::m_ids_manager(0);

                /*Translation manager*/
                const balancer_parameters * translation_manager::m_params = NULL;

                /*Translation servers' manager*/
                const balancer_parameters * translation_servers_manager::m_params = NULL;
                translation_servers_manager::adapters_map translation_servers_manager::m_adapters_data;
                thread * translation_servers_manager::m_re_connect = NULL;
                mutex translation_servers_manager::m_re_connect_mutex;
                condition_variable translation_servers_manager::m_re_connect_condition;
                a_bool_flag translation_servers_manager::m_is_reconnect_run(true);
                mutex translation_servers_manager::m_source_mutex;
                translation_servers_manager::sources_map translation_servers_manager::m_sources;
                default_random_engine translation_servers_manager::m_generator;

            }
        }
    }
}