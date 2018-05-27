/* 
 * File:   client_manager.hpp
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
 * Created on August 1, 2016, 1:09 PM
 */

#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <string>
#include <cstdlib>
#include <chrono>
#include <iostream>

#include "common/utils/threads/threads.hpp"

#include "client/client_consts.hpp"
#include "client/client_parameters.hpp"

#include "common/messaging/websocket/websocket_client_creator.hpp"

using namespace std;
using namespace uva::utils;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::common::messaging::websocket;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                /**
                 * The common base class for the client side managers, e.g. the translation manager and the processor manager.
                 * The main functionality of this class is to send job requests to the provided server and then to receive
                 * job responses from it. When it is finished the response data is requested to be processed.
                 */
                template<msg_type MSG_TYPE, typename RESPONSE_TYPE>
                class client_manager : public websocket_client_creator {
                public:

                    /**
                     * This is the basic constructor
                     * @param params the WebSocket client parameters
                     */
                    client_manager(const websocket_client_params & params)
                    : m_params(params), m_sending_thread_ptr(NULL),
                    m_is_stopping(false), m_is_all_jobs_sent(false),
                    m_is_all_jobs_done(false), m_num_done_jobs(0),
                    m_client(NULL) {
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~client_manager() {
                        //Destroy the translation jobs sending thread
                        if (m_sending_thread_ptr != NULL) {
                            delete m_sending_thread_ptr;
                        }
                        //Destroy the client
                        delete_client();
                    }

                    /**
                     * Allows to start the translation process
                     */
                    inline void start() {
                        //Define and initialize the attempt counter
                        size_t attempt = 0;

                        //Make several re-connection attempts 
                        while (true) {
                            //Create a new client
                            create_client();
                            LOG_INFO1 << "Attempting to connect to the server!" << END_LOG;
                            if (m_client->connect()) {
                                LOG_INFO << "Starting creating and sending out "
                                        << m_params.m_server_name << " jobs!" << END_LOG;

                                //Run the translation job sending thread
                                m_sending_thread_ptr = new thread(bind(&client_manager::send_requests, this));

                                //Stop the loop as we did connect.
                                break;
                            } else {
                                if (attempt >= MAX_NUM_CONNECTION_ATTEMPTS) {
                                    THROW_EXCEPTION(string("Tried ") + to_string(attempt) +
                                            string(" times but could not open the ") +
                                            string("connection to: ") + m_params.m_server_uri);
                                } else {
                                    LOG_INFO2 << "Could not connect, attempt: " << attempt << "/"
                                            << MAX_NUM_CONNECTION_ATTEMPTS << ", timeout: "
                                            << RE_CONNECTION_TIME_OUT_MILLISEC << " milliseconds" << END_LOG;
                                    //Increment the attempts counter
                                    ++attempt;
                                    //Sleep some time before trying to re-connect
                                    std::this_thread::sleep_for(std::chrono::milliseconds(RE_CONNECTION_TIME_OUT_MILLISEC));
                                }
                            }
                        }
                    }

                    /**
                     * Allows to wait until the translations are done
                     */
                    inline void wait() {
                        //Wait until all the jobs are sent
                        {
                            //Make sure that waiting activity is synchronized
                            unique_guard guard(m_jobs_sent_lock);

                            //Wait for the job requests to be sent, use the time out to prevent missing notification
                            while (!m_is_all_jobs_sent &&
                                    (m_jobs_sent_cond.wait_for(guard, chrono::seconds(1)) == cv_status::timeout)) {
                            }
                        }

                        LOG_INFO << "Sent out " << get_act_num_req() << " "
                                << m_params.m_server_name
                                << " jobs, waiting for results." << END_LOG;

                        //Wait until all the jobs are finished or the connection is closed
                        {
                            //Make sure that waiting activity is synchronized
                            unique_guard guard(m_jobs_done_lock);

                            //Wait for the job responses to be received, use the time out to prevent missing notification
                            while (!m_is_all_jobs_done &&
                                    (m_jobs_done_cond.wait_for(guard, chrono::seconds(1)) == cv_status::timeout)) {
                            }
                        }

                        LOG_INFO << "All " << m_params.m_server_name
                                << " job responses are received!" << END_LOG;
                    }

                    /**
                     * This method allows to stop the client and to process the results.
                     */
                    inline void stop() {
                        LOG_INFO << "Stopping the " << m_params.m_server_name
                                << " manager" << END_LOG;

                        //Set the stopping flag
                        m_is_stopping = true;

                        //Wait until the request sending thread is stopped.
                        if (m_sending_thread_ptr != NULL && m_sending_thread_ptr->joinable()) {
                            LOG_DEBUG << "Joining the sending thread" << END_LOG;
                            m_sending_thread_ptr->join();
                        }

                        LOG_INFO << "Disconnecting the " << m_params.m_server_name
                                << " client ... " << END_LOG;

                        //Disconnect from the server
                        if (m_client != NULL) {
                            m_client->disconnect();
                        }

                        LOG_INFO << "The " << m_params.m_server_name
                                << " client is disconnected" << END_LOG;

                        //Process the results
                        process_results();
                    };

                protected:

                    /**
                     * Allows to get the actual number of job requests sent to the server
                     * @return the actual number of job requests sent to the server
                     */
                    virtual size_t get_act_num_req() = 0;

                    /**
                     * Allows to get the expected number of server responses
                     * @return the expected number of server responses
                     */
                    virtual size_t get_exp_num_resp() = 0;

                    /**
                     * Will be called to send the job requests to the server via the provided client.
                     * If could not send, MUST NOT THROW!
                     * @return 
                     */
                    virtual void send_job_requests(websocket_client & client) = 0;

                    /**
                     * Will be called when the message of the expected type arrives from the server.
                     * If could not set the response MUST THROW!
                     * @param job_resp_msg the received job response message
                     */
                    virtual void set_job_response(RESPONSE_TYPE * job_resp_msg) = 0;

                    /**
                     * Will be called when all job responses are received.
                     * If could not process the results MUST THROW!
                     */
                    virtual void process_results() = 0;

                    /**
                     * This function will be called if the connection is closed during the process
                     */
                    inline void notify_conn_closed() {
                        //If the thread was created, I.e. we did establish a connection once!
                        if (m_sending_thread_ptr != NULL) {
                            LOG_WARNING << "The " << m_params.m_server_name
                                    << " server has closed the connection!" << END_LOG;

                            //If the connection is closed we shall be stopping then
                            //The basic client does not support any connection recovery
                            m_is_stopping = true;

                            //Wait until the request sending thread is stopped.
                            if (m_sending_thread_ptr->joinable()) {
                                m_sending_thread_ptr->join();
                            }

                            //Notify that we are done with the jobs
                            notify_jobs_done();
                        }
                    }

                    /**
                     * Allows to notify the threads waiting on the translation jobs to be sent
                     */
                    inline void notify_jobs_sent() {
                        //Make sure that translation-waiting activity is synchronized
                        unique_guard guard(m_jobs_sent_lock);

                        LOG_DEBUG << "Notifying that all the translation job requests are sent ..." << END_LOG;

                        //Setting the translation jobs sent flag 
                        m_is_all_jobs_sent = true;

                        //Notify that the translation is finished
                        m_jobs_sent_cond.notify_all();
                    }

                    /**
                     * Allows to check if all the jobs are done and then perform a notifying action
                     */
                    inline void check_jobs_done_and_notify() {
                        //If we received all the jobs then notify that all the jobs are received!
                        if (m_is_all_jobs_sent && (m_num_done_jobs >= get_exp_num_resp())) {
                            notify_jobs_done();
                        }
                    }

                    /**
                     * Allows to process the server message
                     * @param json_msg a pointer to the json incoming message, not NULL
                     */
                    inline void notify_new_msg(incoming_msg * json_msg) {
                        //Increment the number of received jobs
                        m_num_done_jobs++;

                        //If we are not stopping then set the response
                        if (!m_is_stopping) {
                            //Check on the message type
                            switch (json_msg->get_msg_type()) {
                                case MSG_TYPE:
                                {
                                    //Create a new job response message
                                    RESPONSE_TYPE * job_resp_msg = new RESPONSE_TYPE(json_msg);
                                    try {
                                        //Set the newly received job response
                                        set_job_response(job_resp_msg);
                                    } catch (std::exception & ex) {
                                        LOG_ERROR << ex.what() << END_LOG;
                                        //Delete the message as it was not set
                                        delete job_resp_msg;
                                    }
                                    //Check if the jobs are done and notify
                                    check_jobs_done_and_notify();
                                }
                                    break;
                                default:
                                    THROW_EXCEPTION(string("Unexpected incoming message type: ") +
                                            to_string(json_msg->get_msg_type()));
                            }
                        } else {
                            //Discard the translation job response
                            delete json_msg;
                        }
                    }

                    /**
                     * Allows to notify the threads waiting on the translation jobs to be received
                     */
                    inline void notify_jobs_done() {
                        //Make sure that translation-waiting activity is synchronized
                        unique_guard guard(m_jobs_done_lock);

                        LOG_DEBUG << "Notifying that all the job replies are received ..." << END_LOG;

                        //Setting the translation jobs done flag 
                        m_is_all_jobs_done = true;

                        //Notify that the translation is finished
                        m_jobs_done_cond.notify_all();
                    }

                    /**
                     * This function shall be run in a separate thread and send a number of translation job requests to the server.
                     */
                    inline void send_requests() {
                        LOG_DEBUG << "Sending the job requests ..." << END_LOG;

                        //Send the translation jobs
                        send_job_requests(*m_client);

                        LOG_DEBUG << "Finished sending the job requests ..." << END_LOG;

                        //The translation jobs have been sent!
                        notify_jobs_sent();

                        //For the case when all jobs failed, we need to check if the jobs are notified
                        check_jobs_done_and_notify();
                    }

                    /**
                     * Allows to check if the client is stopping
                     * @return true if the client is stopping, otherwise false.
                     */
                    inline bool is_stopping() {
                        return m_is_stopping;
                    }

                    /**
                     * Allows to create a new client
                     */
                    inline void create_client() {
                        //Delete the previous client if any
                        delete_client();

                        //Create a new client
                        m_client = create_websocket_client(
                                m_params,
                                bind(&client_manager::notify_new_msg, this, _1),
                                bind(&client_manager::notify_conn_closed, this),
                                NULL);
                    }

                    /**
                     * Allows to delete a client if any
                     */
                    inline void delete_client() {
                        if (m_client != NULL) {
                            delete m_client;
                        }
                    }

                private:
                    //Stores the reference to the client configuration parameters
                    const websocket_client_params & m_params;

                    //Stores the synchronization mutex for notifying that all the translation jobs were sent
                    mutex m_jobs_sent_lock;
                    //The conditional variable for tracking that all the translation jobs were sent
                    condition_variable m_jobs_sent_cond;

                    //Stores the synchronization mutex for notifying that all the translation jobs got responces
                    mutex m_jobs_done_lock;
                    //The conditional variable for tracking that all the translation jobs got responces
                    condition_variable m_jobs_done_cond;

                    //Stores the translation request sending thread
                    thread * m_sending_thread_ptr;

                    //Stores the boolean that is used to notify that we need to stop
                    a_bool_flag m_is_stopping;
                    //Stores a flag indicating that all the translation jobs are sent
                    a_bool_flag m_is_all_jobs_sent;
                    //Stores a flag indicating that all the translation jobs are received
                    a_bool_flag m_is_all_jobs_done;

                    //Store the finished jobs count
                    atomic<uint32_t> m_num_done_jobs;

                    //Stores the pointer to the translation client
                    websocket_client * m_client;
                };
            }
        }
    }
}


#endif /* CLIENT_MANAGER_HPP */

