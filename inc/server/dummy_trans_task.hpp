/* 
 * File:   dummy_trans_job.hpp
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
 * Created on January 20, 2016, 6:41 PM
 */

#include <cstdlib>        // srd::rand
#include <thread>         // std::thread std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "common/utils/Exceptions.hpp"
#include "common/utils/logging/Logger.hpp"

#include "trans_session.hpp"
#include "trans_task.hpp"
#include "common/messaging/trans_job_request.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::smt::decoding::common::messaging;

#ifndef DUMMY_TRANS_JOB_HPP
#define DUMMY_TRANS_JOB_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {
                namespace dummy {
                    
                    //Define the pointer to the dummy translation task
                    class dummy_trans_task;
                    typedef dummy_trans_task * dummy_trans_task_ptr;

                    /**
                     * This is a dummy translation task class that is used for the sake of testing only.
                     * This class inherits from a thread and 
                     */
                    class dummy_trans_task : public thread {
                    public:
                        typedef websocketpp::lib::function<void(const task_id_type task_id, const session_id_type session_id, const job_id_type job_id, const string & text) > task_result_reporter;

                        //The maximum time the dummy task will be doing translation
                        static constexpr uint32_t MAX_RUN_SEC = 2 * 60; // 2 minutes
                        //The maximum time the dummy task will be waiting to start
                        static constexpr uint32_t MAX_WAIT_SEC = 60; // 1 minute

                        /**
                         * This is a basic constructor that receives the 
                         * @param task_id the translation task id
                         * @param session_id the translation task id
                         * @param request the pointer to the translation job request, not NULL
                         */
                        dummy_trans_task(const task_id_type task_id, const session_id_type session_id, const trans_job_request_ptr request,
                                task_result_reporter sender_func)
                        : thread(bind(&dummy_trans_task::run_simulation, this)),
                        m_is_interrupted(false), m_task_id(task_id), m_session_id(session_id),
                        m_request(request), m_report_result_func(sender_func) {
                            //Do the sanity check asserts
                            ASSERT_SANITY_THROW((m_request == NULL), "Received a NULL pointer translation request!");
                            ASSERT_SANITY_THROW(!m_report_result_func,
                                    "The sender function of the dummy translation task is not set!");
                        }

                        /**
                         * The basic destructor of the dummy translation job
                         */
                        virtual ~dummy_trans_task() {
                            if (m_request != NULL) {
                                delete m_request;
                            }
                        }

                        /**
                         * Allows to stop the translation job simulation
                         */
                        void stop_simulation() {
                            //Set the interrupted flag
                            m_is_interrupted = true;
                        }

                    protected:

                        /**
                         * Allows to run the thread with periodical sleeps, each second
                         * one will check if the thread is interrupted, if yes then it will stop.
                         * The time the method will execute is random and is limited by the
                         * method's argument value +/- overhead.
                         * @param max_dur_sec the maximum time in seconds to be run.
                         */
                        template<uint32_t MAX_DUR_SEC>
                        void run_interruptable_job() {
                            if (!m_is_interrupted) {
                                const uint32_t time_sec = rand() % MAX_DUR_SEC;
                                for (uint32_t i = 0; i <= time_sec; ++i) {
                                    if (m_is_interrupted) break;
                                    this_thread::sleep_for(chrono::seconds(1));
                                }
                            }
                        }

                        /**
                         * Allows to start the translation job simulation
                         */
                        void run_simulation() {
                            // 1. The waiting sleep that just sleeps for a random
                            //    number of milliseconds from a range, this should
                            //    emulate the delay before the translation is started
                            run_interruptable_job<MAX_WAIT_SEC>();

                            // 2. The main loop that emulates the translation job execution.
                            //    This is to emulate the actual decoding process.
                            run_interruptable_job<MAX_RUN_SEC>();

                            // 3. Send the response to the client
                            m_report_result_func(m_task_id, m_session_id, m_request->get_job_id(), string("Translation for: ") + m_request->get_text());
                        }

                    private:
                        //Stores the interrupted flag
                        bool m_is_interrupted;
                        //Stores the translation task id
                        const task_id_type m_task_id;
                        //Stores the session id
                        const session_id_type m_session_id;
                        //Stores the pointer to the translation job request, not NULL
                        const trans_job_request_ptr m_request;
                        //Stores the response setter
                        const task_result_reporter m_report_result_func;
                    };

                    constexpr uint32_t dummy_trans_task::MAX_RUN_SEC;
                    constexpr uint32_t dummy_trans_task::MAX_WAIT_SEC;
                }
            }
        }
    }
}

#endif /* DUMMY_TRANS_JOB_HPP */

