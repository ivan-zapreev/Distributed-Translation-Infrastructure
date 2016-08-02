/* 
 * File:   proc_resp_in.hpp
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
 * Created on August 2, 2016, 9:19 AM
 */

#ifndef PROC_RESP_IN_HPP
#define PROC_RESP_IN_HPP

#include "common/messaging/proc_resp.hpp"
#include "common/messaging/incoming_msg.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {
                    
                    /**
                     * This class represents the incoming text processor response.
                     */
                    class proc_resp_in : public proc_resp {
                    public:

                        /**
                         * The basic constructor
                         * @param inc_msg the pointer to the incoming message, NOT NULL
                         */
                        proc_resp_in(const incoming_msg * inc_msg)
                        : proc_resp(), m_inc_msg(inc_msg) {
                            //Nothing to be done here
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~proc_resp_in() {
                            //Destroy the incoming message, the pointer must not be NULL
                            delete m_inc_msg;
                        }

                    protected:
                        //Stores the pointer to the incoming message
                        const incoming_msg * m_inc_msg;

                    };
                    
                    //Typedef the pointer to the request
                    typedef proc_resp_in * proc_resp_in_ptr;
                }
            }
        }
    }
}

#endif /* PROC_RESP_IN_HPP */

