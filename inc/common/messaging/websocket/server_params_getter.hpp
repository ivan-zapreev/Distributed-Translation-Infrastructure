/* 
 * File:   websocket_server_params_getter.hpp
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
 * Created on May 28, 2018, 12:40 PM
 */

#ifndef WEBSOCKET_SERVER_PARAMS_GETTER_HPP
#define WEBSOCKET_SERVER_PARAMS_GETTER_HPP

/**
 * Allows to get the server-related TLS parameters
 * @param ini the ini file to get the parameters from
 * @param section the section to get them from
 * @param ws_params the parameters structure to be filled in
 */
static inline void get_tls_server_params(
        INI<> & ini, const string & section,
        websocket_server_params& ws_params) {
    //Process the TLS related parameters
    ws_params.m_is_tls_server = get_bool(ini, section,
            websocket_server_params::SE_IS_TLS_SERVER_PARAM_NAME, "false", IS_TLS_SUPPORT);
    if (IS_TLS_SUPPORT && ws_params.m_is_tls_server) {
        //The remaining TLS parameters are only relevant if the TLS is supported and requested
        ws_params.m_tls_mode_name = get_string(ini, section,
                websocket_server_params::SE_TLS_MODE_PARAM_NAME, "", ws_params.m_is_tls_server);
        const tls_mode_enum tls_mode = tls_str_to_val(ws_params.m_tls_mode_name);
        if (tls_mode != tls_mode_enum::MOZILLA_UNDEFINED) {
            //If the TLS mode is undefined then this will be an 
            //error during finalization, for not we assume it is defined!
            ws_params.m_tls_crt_file = get_string(ini, section,
                    websocket_server_params::SE_TLS_CRT_FILE_PARAM_NAME);
            ws_params.m_tls_key_file = get_string(ini, section,
                    websocket_server_params::SE_TLS_KEY_FILE_PARAM_NAME);
            ws_params.m_tls_dh_file = get_string(ini, section,
                    websocket_server_params::SE_TLS_DH_FILE_PARAM_NAME);
        }
    }
}


#endif /* WEBSOCKET_SERVER_PARAMS_GETTER_HPP */

