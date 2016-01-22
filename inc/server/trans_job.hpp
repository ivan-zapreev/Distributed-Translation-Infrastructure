/* 
 * File:   trans_job.hpp
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

#include "trans_task.hpp"
#include "common/messaging/trans_session.hpp"
#include "common/messaging/trans_job_request.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

#ifndef TRANS_JOB_HPP
#define	TRANS_JOB_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {

                //Declare the translation job pointer
                class trans_job;
                typedef trans_job* trans_job_ptr;

                /**
                 * This class represents the translation job. Each translation job
                 * belongs to a session and contains a translation request.
                 * Every translation request is a text consisting of multiple sentences.
                 * The translation job therefore splits this request into a number
                 * of translation tasks each of which translates one sentence.
                 */
                class trans_job {
                public:
                    typedef websocketpp::lib::function<void(const trans_job &) > done_notifier;
                    typedef vector<trans_task_ptr> tasks_list_type;
                    typedef tasks_list_type::iterator tasks_iter_type;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param job_id the translation job id
                     * @param task_ids the list of task ids from which this job consists of
                     */
                    trans_job(trans_job_request_ptr request_ptr) : m_request_ptr(request_ptr) {
                        //ToDo: Split the translation request into a number of translation tasks
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_job() {
                        //If the job is deleted then the request is not needed any more
                        if (m_request_ptr != NULL) {
                            delete m_request_ptr;
                        }

                        //Delete the translation tasks
                        for (tasks_iter_type it = m_tasks.begin(); it != m_tasks.end(); ++it) {
                            delete *it;
                        }
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    const session_id_type get_session_id() const {
                        return m_request_ptr->get_session_id();
                    }

                    /**
                     * Allows to retrieve the job id
                     * @return the job id
                     */
                    const job_id_type get_job_id() const {
                        return m_request_ptr->get_job_id();
                    }

                    /**
                     * Allows to get the list of translation tasks
                     * @return the list of translation tasks of this job
                     */
                    vector<trans_task_ptr>& get_tasks() {
                        return m_tasks;
                    }

                    /**
                     * Allows to retrieve the translation task result code
                     * @return the translation task result code
                     */
                    virtual const trans_job get_code() const {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * Allows to retrieve the translation task result text
                     * @return the translation task result text
                     */
                    virtual const string & get_text() const {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                protected:

                    /**
                     * Is used from the translation task to notify the translation job that the task is ready
                     * @param task the translation task that is finished
                     */
                    void notify_task_ready(const trans_task& task) {
                        //ToDo: Implement
                    }

                private:
                    //Stores the original translation request
                    trans_job_request_ptr m_request_ptr;

                    //Stores the list of translation tasks of this job
                    tasks_list_type & m_tasks;
                };
            }
        }
    }
}

#endif	/* TRANS_JOB_RESULT_HPP */

