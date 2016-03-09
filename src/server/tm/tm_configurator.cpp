/* 
 * File:   tm_configurator.cpp
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
 * Created on February 08, 2016, 09:38 AM
 */

#include "server/tm/tm_configurator.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    //Just give a default initialization
                    const tm_parameters * tm_configurator::m_params = NULL;
                    
                    //Just give a default initialization
                    tm_proxy * tm_configurator::m_model_proxy = NULL;
                }
            }
        }
    }
}