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

#ifndef DUMMY_TRANS_JOB_HPP
#define DUMMY_TRANS_JOB_HPP

#include <cstdlib>        // srd::rand
#include <chrono>         // std::chrono::seconds

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads.hpp"

#include "trans_task.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_request.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::smt::decoding::common::messaging;

namespace uva {
    namespace smt {
        namespace decoding {
            namespace server {
                namespace dummy {

                    //Define the pointer to the dummy decoder
                    class decoder_stub;
                    typedef decoder_stub * decoder_stub_ptr;

                    /**
                     * This is a dummy decoder class that is used for the sake of testing only.
                     * This class inherits from a thread and represents a decoder instance that
                     * is supposed to execute a translation job for a sentence.
                     */
                    class decoder_stub : public thread {
                    public:

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
                        decoder_stub(const task_id_type task_id, const session_id_type session_id, const trans_job_request_ptr request,
                                task_result_setter sender_func)
                        : thread(bind(&decoder_stub::run_simulation, this)),
                        m_is_interrupted(false), m_trans_result(session_id, request->get_job_id(), task_id),
                        m_request(request), m_set_task_result_func(sender_func) {
                            //Do the sanity check asserts
                            ASSERT_SANITY_THROW((m_request == NULL), "Received a NULL pointer translation request!");
                            ASSERT_SANITY_THROW(!m_set_task_result_func,
                                    "The sender function of the dummy translation task is not set!");
                        }

                        /**
                         * The basic destructor of the dummy translation job
                         */
                        virtual ~decoder_stub() {
                            if (m_request != NULL) {
                                delete m_request;
                            }
                        }

                        /**
                         * Allows to stop the translation job simulation
                         */
                        void cancel() {
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
                            //The waiting sleep that just sleeps for a random
                            //number of milliseconds from a range, this should
                            //emulate the delay before the translation is started
                            run_interruptable_job<MAX_WAIT_SEC>();

                            //The main loop that emulates the translation job execution.
                            //This is to emulate the actual decoding process.
                            run_interruptable_job<MAX_RUN_SEC>();

                            //Set the data into the translation result, depending
                            //on whether we were interrupted or not, to it synchronously
                            if (m_is_interrupted) {
                                m_trans_result.set_result(trans_job_code::RESULT_ERROR, "Is interrupter!");
                            } else {
                                m_trans_result.set_result(trans_job_code::RESULT_OK, "Translated text!");
                            }

                            //Send the response to the client
                            m_set_task_result_func(m_trans_result);
                        }

                    private:
                        //Stores the interrupted flag
                        bool m_is_interrupted;
                        //Stores the translation result 
                        trans_task m_trans_result;
                        //Stores the pointer to the translation job request, not NULL
                        const trans_job_request_ptr m_request;
                        //Stores the response setter
                        const task_result_setter m_set_task_result_func;
                    };

                    constexpr uint32_t decoder_stub::MAX_RUN_SEC;
                    constexpr uint32_t decoder_stub::MAX_WAIT_SEC;
                }
            }
        }
    }
}

#endif /* DUMMY_TRANS_JOB_HPP */

