/* 
 * File:   trans_job_result.hpp
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
 * Created on January 21, 2016, 4:08 PM
 */

#include <string>

#include "abs_trans_result.hpp"
#include "common/messaging/trans_job_result.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef TRANS_JOB_RESULT_HPP
#define	TRANS_JOB_RESULT_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                /**
                 * This class represents the translation job result
                 */
                class trans_job_result : public abs_trans_result {
                public:

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param job_id the translation job id
                     * @param task_ids the list of task ids from which this job consists of
                     */
                    trans_job_result(const session_id_type session_id, const job_id_type job_id)
                    : abs_trans_result(session_id, job_id) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_job_result() {
                        //Nothing to be done
                    }

                    /**
                     * Allows to set the translation result
                     * @param code the result code
                     * @param text the resulting text, or an error message if there was an error
                     */
                    void set_result(const trans_job_result code, const string & text) {
                        m_code = code;
                        m_text = text;
                    }

                    /**
                     * Allows to retrieve the task id
                     * @return the task id
                     */
                    const task_id_type get_task_id() const {
                        return m_task_id;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    virtual const trans_job_result get_code() const {
                        return m_code;
                    }

                    /**
                     * Allows to retrieve the translation task result text
                     * @return the translation task result text
                     */
                    virtual const string & get_text() const {
                        return m_text;
                    }

                private:
                    //Stores the translation task id
                    const task_id_type m_task_id;

                    //Stores the translation task result code
                    trans_job_result m_code;

                    //Stores the translation task result text, error message or the translated sentence
                    string m_text;
                };
            }
        }
    }
}

#endif	/* TRANS_JOB_RESULT_HPP */

