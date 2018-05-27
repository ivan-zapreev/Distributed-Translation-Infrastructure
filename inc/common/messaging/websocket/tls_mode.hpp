/* 
 * File:   tls_mode.hpp
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
 * Created on May 25, 2018, 4:07 PM
 */

#ifndef TLS_MODE_HPP
#define TLS_MODE_HPP

#include <string>

#include "common/utils/text/string_utils.hpp"

using namespace std;

using namespace uva::utils::text;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {
                    namespace websocket {

                        /* This enumeration defined the TLS mode
                         * See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
                         * the TLS modes. The code below demonstrates how to implement both the modern
                         */
                        enum tls_mode_enum {
                            MOZILLA_UNDEFINED = 0,
                            MOZILLA_OLD = MOZILLA_UNDEFINED + 1,
                            MOZILLA_INTERMEDIATE = MOZILLA_OLD + 1,
                            MOZILLA_MODERN = MOZILLA_INTERMEDIATE + 1
                        };

                        /**
                         * Allows to convert the tls mode value into a string
                         * @param mode the tls mode value
                         * @return the corresponding string
                         */
                        inline string tls_val_to_str(const tls_mode_enum mode) {
                            switch (mode) {
                                case tls_mode_enum::MOZILLA_OLD:
                                    return string("old");
                                case tls_mode_enum::MOZILLA_INTERMEDIATE:
                                    return string("int");
                                case tls_mode_enum::MOZILLA_MODERN:
                                    return string("mod");
                                default:
                                    return string("undef");
                            }
                        }

                        /**
                         * Allows to convert the MOZILLA TLS mode name
                         * (old, intermediate, modern) into a the corresponding TLS mode.
                         * @param tls_str the TLS name string
                         * @return the corresponding TLS mode or MOZILLA_UNDEFINED if not recognized
                         */
                        inline tls_mode_enum tls_str_to_val(string tls_str) {
                            //Turn into trimmed low-case string
                            to_lower(trim(tls_str));
                            //Compare with constant literals
                            if (tls_str.compare(tls_val_to_str(tls_mode_enum::MOZILLA_OLD)) == 0) {
                                return tls_mode_enum::MOZILLA_OLD;
                            } else {
                                if (tls_str.compare(tls_val_to_str(tls_mode_enum::MOZILLA_INTERMEDIATE)) == 0) {
                                    return tls_mode_enum::MOZILLA_INTERMEDIATE;
                                } else {
                                    if (tls_str.compare(tls_val_to_str(tls_mode_enum::MOZILLA_MODERN)) == 0) {
                                        return tls_mode_enum::MOZILLA_MODERN;
                                    } else {
                                        return tls_mode_enum::MOZILLA_UNDEFINED;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif /* TLS_MODE_HPP */

