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
#include <vector>

#include <websocketpp/common/thread.hpp>

#include "trans_task.hpp"
#include "common/messaging/id_manager.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_id.hpp"

using namespace std;
using namespace uva::smt::decoding::common::messaging;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;

#ifndef TRANS_JOB_HPP
#define TRANS_JOB_HPP

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
                    //Define the lock type to synchronize map operations
                    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                    //Define the function type for the function used to set the translation job resut
                    typedef websocketpp::lib::function<void(trans_job_ptr trans_job) > done_job_notifier;

                    //Declare the tasks list type and its iterator
                    typedef vector<trans_task_ptr> tasks_list_type;
                    typedef tasks_list_type::iterator tasks_iter_type;

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param job_id the translation job id
                     * @param task_ids the list of task ids from which this job consists of
                     */
                    trans_job(trans_job_request_ptr request_ptr)
                    : m_request_ptr(request_ptr), m_done_tasks_count(0) {
                        //Get the text to be translated
                        string text = m_request_ptr->get_text();
                        //Obtain the text to be parsed
                        TextPieceReader reader(text.c_str(), text.length());
                        //This reader will store the read text sentence
                        TextPieceReader sentence;

                        //Read the text line by line, each line must be one sentence
                        //to translate. For each read line create a translation task.
                        while (reader.get_first<trans_job_request::TEXT_SENTENCE_DELIMITER>(sentence)) {
                            m_tasks.push_back(new trans_task(m_id_mgr.get_next_id(), sentence.str(),
                                    bind(&trans_job::notify_task_done, this, _1)));
                        }
                    }

                    /**
                     * Allows to set the function that should be called when the job is done
                     * @param notify_job_done_func
                     */
                    void set_done_job_notifier(done_job_notifier notify_job_done_func) {
                        m_notify_job_done_func = notify_job_done_func;
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
                    virtual const trans_job_code get_code() const {
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

                    /**
                     * Allows to cancel the given translation job
                     */
                    void cancel() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                protected:

                    /**
                     * Allows to check if the job is finished by checking the number
                     * of finished tasks. The check is synchronized.
                     * @return true if all the job's tasks are finished, otherwise false
                     */
                    bool is_job_finished() {
                        scoped_lock guard_tasks(m_tasks_lock);

                        return (m_done_tasks_count == m_tasks.size());
                    }

                    /**
                     * Is used from the translation task to notify the translation
                     * job that the task is ready. This method is thread safe
                     * @param task the translation task that is finished
                     */
                    void notify_task_done(const trans_task_ptr& task) {
                        scoped_lock guard_tasks(m_tasks_lock);

                        //ToDo: Do a strict check on the tasks reporting to be finished,
                        //these should be the onsed from the m_tasks list and they must
                        //report themselves only ones.

                        //Increment the finished tasks count
                        m_done_tasks_count++;

                        //If all the tasks are translated then we notify that this job id done
                        if (is_job_finished()) {
                            m_notify_job_done_func(this);
                        }
                    }

                private:
                    //Stores the synchronization mutex for working with the m_sessions_map
                    websocketpp::lib::mutex m_tasks_lock;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //Stores the original translation request
                    trans_job_request_ptr m_request_ptr;

                    //Stores the list of translation tasks of this job
                    tasks_list_type m_tasks;

                    //The count of the finished tasks
                    atomic<uint32_t> m_done_tasks_count;

                    //Stores the static instance of the id manager
                    static id_manager<task_id_type> m_id_mgr;
                };

                id_manager<task_id_type> trans_job::m_id_mgr(task_id::MINIMUM_TASK_ID);
            }
        }
    }
}

#endif /* TRANS_JOB_RESULT_HPP */

