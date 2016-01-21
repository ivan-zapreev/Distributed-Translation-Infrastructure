/* 
 * File:   abs_trans_result.hpp
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
 * Created on January 21, 2016, 3:59 PM
 */

#include <string>

#include "common/messaging/trans_job_result.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef ABS_TRANS_RESULT_HPP
#define	ABS_TRANS_RESULT_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This class represents the abstract translation result.
                 */
                class abs_trans_result {
                public:

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param job_id the translation job id
                     */
                    abs_trans_result(const session_id_type session_id, const job_id_type job_id)
                    : m_session_id(session_id), m_job_id(job_id) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~abs_trans_result() {
                        //Nothing to be done
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    const session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the job id
                     * @return the job id
                     */
                    const job_id_type get_job_id() const {
                        return m_job_id;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    virtual const trans_job_result get_code() const = 0;

                    /**
                     * Allows to retrieve the translation task result text
                     * @return the translation task result text
                     */
                    virtual const string & get_text() const  = 0;

                private:
                    //Stores the session id
                    const session_id_type m_session_id;
                    
                    //Stores the client issued job_id 
                    const job_id_type m_job_id;
                };
            }
        }
    }
}


#endif	/* ABS_TRANS_RESULT_HPP */

