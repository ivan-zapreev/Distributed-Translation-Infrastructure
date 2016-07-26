/* 
 * File:   pre_proc_req.hpp
 * Author: zapreevis
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
 * Created on July 26, 2016, 11:02 AM
 */

#ifndef PRE_PROC_REQ_HPP
#define	PRE_PROC_REQ_HPP

#include <common/messaging/request_msg.hpp>

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    /**
                     * This is the base class for all the text pre-process request messages
                     */
                    class pre_proc_req : public request_msg {
                    public:

                        /**
                         * The basic constructor
                         */
                        pre_proc_req() : request_msg() {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~pre_proc_req() {
                            //Nothing to be done here
                        }
                    };
                }
            }
        }
    }
}

#endif	/* PRE_PROC_REQ_HPP */

