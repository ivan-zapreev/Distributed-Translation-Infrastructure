/* 
 * File:   trans_job_id.hpp
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
 * Created on January 22, 2016, 3:53 PM
 */

#ifndef TRANS_JOB_ID_HPP
#define TRANS_JOB_ID_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace common {
                namespace messaging {

                    //Make the typedef for the translation job id
                    typedef uint64_t job_id_type;

                    namespace job_id {
                        //Stores the undefined value of the translation job id
                        static constexpr job_id_type UNDEFINED_JOB_ID = 0;
                        //Stores the minimum allowed translation job id
                        static constexpr job_id_type MINIMUM_JOB_ID = 1;
                    }
                }
            }
        }
    }
}

#endif /* TRANS_JOB_ID_HPP */

