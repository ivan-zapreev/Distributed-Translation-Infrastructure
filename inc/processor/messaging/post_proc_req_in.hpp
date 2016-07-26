/* 
 * File:   post_proc_req_in.hpp
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
 * Created on July 26, 2016, 10:42 AM
 */

#ifndef POST_PROC_REQ_IN_HPP
#define	POST_PROC_REQ_IN_HPP

#include "common/messaging/language_registry.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/post_proc_req.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                namespace messaging {
                    
                    /**
                     * This class represents the incoming text post-process request.
                     */
                    class post_proc_req_in : public post_proc_req {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        post_proc_req_in(const incoming_msg * inc_msg)
                        : post_proc_req(), m_inc_msg(inc_msg) {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~post_proc_req_in() {
                            //Destroy the incoming message, the pointer must not be NULL
                            delete m_inc_msg;
                        }

                    private:
                        //Stores the pointer to the incoming message
                        const incoming_msg * m_inc_msg;
                    };
                }
            }
        }
    }
}

#endif	/* POST_PROC_REQ_IN_HPP */

