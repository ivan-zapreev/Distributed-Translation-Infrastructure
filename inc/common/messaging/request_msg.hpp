/* 
 * File:   request_msg.hpp
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

#ifndef REQUEST_MSG_HPP
#define REQUEST_MSG_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the base class for all the request messages
                     */
                    class request_msg {
                    public:

                        /**
                         * The basic constructor
                         */
                        request_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~request_msg() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* REQUEST_MSG_HPP */

