/* 
 * File:   client_params_getter.hpp
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
 * Created on May 29, 2018, 11:45 AM
 */

#ifndef CLIENT_PARAMS_GETTER_HPP
#define CLIENT_PARAMS_GETTER_HPP

#include "common/messaging/websocket/websocket_client_params.hpp"

/**
 * Allows to get the client-related parameters
 * @param IS_URI_OPTIONAL if true then the URI is optional.
 * @param ini the ini file to get the parameters from
 * @param section the section to get them from
 * @param wc_params the parameters structure to be filled in
 * @param def_uri the default URI value, for when 'IS_URI_OPTIONAL == true'
 */
template<bool IS_URI_OPTIONAL = false >
static inline void get_client_params(
        INI<> & ini, const string & section,
        websocket_client_params& cs_params,
        const string def_uri = "") {
    //Get server URI
    cs_params.m_server_uri = get_string(ini, section,
            websocket_client_params::WC_SERVER_URI_PARAM_NAME,
            def_uri, !IS_URI_OPTIONAL);

    //If the URI is not optional or it is optional but is not default value
    if (!IS_URI_OPTIONAL || (cs_params.m_server_uri != def_uri)) {
        //Read TLS related parameters only if this is a TLS server URI
        if (IS_TLS_SUPPORT && websocket_client_params::is_tls_server(cs_params.m_server_uri)) {
            //The TLS mode is required if this is a TLS server URI
            cs_params.m_tls_mode_name = get_string(ini, section,
                    websocket_client_params::WC_TLS_MODE_PARAM_NAME);
            //Ciphers are an optional parameter that, if misused can cause TLS handshake failure!
            cs_params.m_tls_ciphers = get_string(ini, section,
                    websocket_client_params::WC_TLS_CIPHERS_PARAM_NAME, "", false);
        }
    }
}

#endif /* CLIENT_PARAMS_GETTER_HPP */

