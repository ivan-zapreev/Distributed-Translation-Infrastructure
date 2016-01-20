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

                    /**
                     * This is a dummy translation task class that is used for the sake of testing only.
                     * This class inherits from a thread and 
                     */
                    class dummy_trans_task : public thread {
                    public:
                        typedef websocketpp::lib::function<void(const session_id_type session_id, const job_id_type job_id, const string & text) > response_sender;

                        /**
                         * This is a basic constructor that receives the 
                         * @param session_id the translation task id
                         * @param request the pointer to the translation job request, not NULL
                         */
                        dummy_trans_task(const session_id_type session_id, const trans_job_request_ptr request,
                                response_sender send_response)
                        : thread(bind(&dummy_trans_task::run_simulation, this)),
                        is_interrupted(false), m_session_id(session_id), m_request(request), m_send_response(send_response) {
                            ASSERT_SANITY_THROW((m_request == NULL), "Received a NULL pointer translation request!");
                        }

                        /**
                         * The basic destructor of the dummy translation job
                         */
                        virtual ~dummy_trans_task() {
                            delete m_request;
                        }

                        /**
                         * Allows to stop the translation job simulation
                         */
                        void stop_simulation() {
                            //Set the interrupted flag
                            is_interrupted = true;
                        }

                    protected:

                        /**
                         * Allows to start the translation job simulation
                         */
                        void run_simulation() {
                            // 1. The waiting sleep that just sleeps for a random
                            //    number of milliseconds from a range, this should
                            //    emulate the delay before the translation is started
                            this_thread::sleep_for(chrono::seconds(1));

                            // 2. The main loop that emulates the translation job execution.
                            //    This is to emulate the actual decoding process.
                            if (!is_interrupted) {
                                const uint32_t num_iterations = rand() % 100;
                                for (uint32_t i = 0; i <= num_iterations; ++i) {
                                    if (is_interrupted) break;
                                    this_thread::sleep_for(chrono::seconds(1));
                                }
                            }

                            // 3. Send the response to the client
                            m_send_response(m_session_id, m_request->get_job_id(), string("Translation for: ") + m_request->get_text());
                        }

                    private:
                        //Stores the interrupted flag
                        bool is_interrupted;
                        //Stores the session id
                        const session_id_type m_session_id;
                        //Stores the pointer to the translation job request, not NULL
                        const trans_job_request_ptr m_request;
                        //Stores the response setter
                        const response_sender m_send_response;
                    };
                }
            }
        }
    }
}

#endif /* DUMMY_TRANS_JOB_HPP */

