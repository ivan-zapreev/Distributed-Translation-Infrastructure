/* 
 * File:   balancer_job.hpp
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
 * Created on July 19, 2016, 11:19 PM
 */

#include "balancer/balancer_job.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                id_manager<job_id_type> balancer_job::m_id_mgr(job_id::MINIMUM_JOB_ID);

                /**
                 * Allows to log the balancer job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const balancer_job & job) {
                    return stream << "{session_id: " << to_string(job.m_session_id)
                            << ", job_id: " << to_string(job.m_job_id)
                            << ", bal_job_id: " << to_string(job.m_bal_job_id)
                            << ", phase: " << to_string(job.m_phase)
                            << ", state: " << to_string(job.m_state)
                            << ", adapter_uid: " << to_string(job.m_adapter_uid)
                            << ", err_msg: \'" << job.m_err_msg << "\'}";
                }

            }
        }
    }
}
