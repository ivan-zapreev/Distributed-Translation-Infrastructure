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
 * Created on July 7, 2016, 12:11 PM
 */

#ifndef BALANCER_JOB_HPP
#define BALANCER_JOB_HPP

#include <ostream>

#include "common/utils/id_manager.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "common/messaging/msg_base.hpp"
#include "common/messaging/trans_session_id.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/status_code.hpp"

#include "client/messaging/trans_job_req_out.hpp"
#include "client/messaging/trans_job_resp_in.hpp"
#include "server/messaging/trans_job_req_in.hpp"
#include "server/messaging/trans_job_resp_out.hpp"

#include "balancer/translator_adapter.hpp"

using namespace std;

using namespace uva::utils;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::client::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                //Forward class declaration
                class balancer_job;
                //Typedef the balancer job pointer
                typedef balancer_job * bal_job_ptr;

                /**
                 * Allows to log the balancer job into an output stream
                 * @param stream the output stream
                 * @param job the job to log
                 * @return the reference to the same output stream send back for chaining
                 */
                ostream & operator<<(ostream & stream, const balancer_job & job);

                //Declare the function that will be used to send the translation response to the client
                typedef function<bool (const session_id_type, const msg_base &) > session_response_sender;

                //Declare the function that will be choosing the proper adapter for the translation job
                typedef function<translator_adapter *(const trans_job_req_in *) > adapter_chooser;

                //Declare the function type that is of a general purpose, is used to notify something about the job
                typedef function<void(const balancer_job *) > job_notifier;

                /**
                 * This is the translation job class:
                 * Responsibilities:
                 *      Stores the session id/handler
                 *      Stores the original job id
                 *      Stores the newly issued job id
                 *      Stores the text to be translated
                 *      Stores the translation job response
                 *      Gets the server adapter
                 *      Notify about a failed job dispatch
                 *      Send translation request
                 *      Send the translation response
                 *      Remember in which state the job is:
                 *         Waiting for sending request
                 *         Waiting for receiving reply
                 */
                class balancer_job {
                public:

                    //Define the function type for the function used to set the translation job result
                    typedef function<void(bal_job_ptr) > done_job_notifier;
                    //Define the function that is to be called to remove the task from the pool
                    typedef function<void(bal_job_ptr) > task_pool_remover;

                    /**
                     * This enumeration stores the balancer job internal states
                     */
                    enum phase {
                        UNDEFINED_PHASE = 0, //When created the phase is not initialized - undefined
                        REQUEST_PHASE = 1, //The balancer job is created and initialized, the request is present, waiting to be send to the translator
                        RESPONSE_PHASE = 2, //The translation request is sent to the translator, waiting for the translator's response
                        REPLY_PHASE = 3, //The translator's response is received waiting until the reply will be sent to the client
                        DONE_PHASE = 4, //The reply is sent to the client.
                    };

                    /**
                     * This enumeration stores the balancer job internal states
                     */
                    enum state {
                        UNDEFINED_STATE = 0, //When created the state is not initialized - undefined
                        ACTIVE_STATE = 1, //The state is active, i.e. the job is not canceled an there was not failure
                        CANCELED_STATE = 5, //The job is canceled, due to a client session disconnect
                        FAILED_STATE = 6 //The job is failed, due to a translator's adapter disconnect
                    };

                    /**
                     * The basic constructor allowing to initialize the main class constants
                     * @param session_id the id of the session from which the translation request is received
                     * @param trans_req the reference to the translation job request to get the data from.
                     * @param chooser_func the function to choose the adapter, not NULL
                     * @param register_wait_func the function to register the job as awaiting response, not NULL
                     * @param notify_err_func the function to register the job as having received an error response, not NULL
                     * @param resp_send_func the function to send the translation response to the client
                     */
                    balancer_job(const session_id_type session_id, trans_job_req_in * trans_req,
                            const adapter_chooser & chooser_func, const job_notifier & register_wait_func,
                            const job_notifier & notify_err_func, const session_response_sender & resp_send_func)
                    : m_session_id(session_id), m_job_id(trans_req->get_job_id()),
                    m_trans_req(trans_req), m_trans_resp(NULL),
                    m_notify_job_done_func(NULL), m_choose_adapt_func(chooser_func),
                    m_register_wait_func(register_wait_func), m_notify_err_func(notify_err_func),
                    m_resp_send_func(resp_send_func), m_phase(phase::REQUEST_PHASE),
                    m_state(state::ACTIVE_STATE), m_err_msg(""), m_bal_job_id(m_id_mgr.get_next_id()),
                    m_adapter_uid(server_uid::UNDEFINED_SERVER_ID) {
                    }

                    /**
                     * The basic constructor
                     */
                    virtual ~balancer_job() {
                        //Destroy the translation request if present
                        if (m_trans_req != NULL) {
                            delete m_trans_req;
                        }
                        //Destroy the translation response if present
                        if (m_trans_resp != NULL) {
                            delete m_trans_resp;
                        }
                    }

                    /**
                     * Allows to retrieve the session id
                     * @return the session id
                     */
                    inline session_id_type get_session_id() const {
                        return m_session_id;
                    }

                    /**
                     * Allows to retrieve the job id as given by the client.
                     * The job id given by the balancer is retrieved by another method.
                     * @return the job id
                     */
                    inline job_id_type get_job_id() const {
                        return m_job_id;
                    }

                    /**
                     * Allows to retrieve the job id as given by the balancer
                     * (to be used in the request to the translation server).
                     * The original job id given by the client is retrieved by
                     * another method.
                     * @return the new unique job id issued by the balancer
                     */
                    inline job_id_type get_bal_job_id() const {
                        return m_bal_job_id;
                    }

                    /**
                     * Allows to set the function that should be called when the job is done
                     * @param notify_job_done_func the job done notification function
                     *              to be called when the translation job is finished
                     */
                    inline void set_done_job_notifier(done_job_notifier notify_job_done_func) {
                        m_notify_job_done_func = notify_job_done_func;
                    }

                    /**
                     * Allows to set in the method that shall remove the task from the pool if called
                     * @param notify_task_cancel_func the function to call in case this task is being canceled.
                     */
                    inline void set_from_pool_remover(task_pool_remover pool_task_remove_func) {
                        //Do not store the function, we do not want to remove it from the pool.
                        //This job, even if canceled will be executed by the pool's worker
                        //and will be removed from the pool by him as well. 
                    }

                    /**
                     * Allows to get the unique identifier of the translation server adapter.
                     * Is only set in case the request was attempted to be sent.
                     * I.e. after the first execution.
                     * @return the name of the server
                     */
                    inline const server_uid_type & get_server_uid() const {
                        return m_adapter_uid;
                    }

                    /**
                     * Stores the pointer to the incoming translation job response.
                     * @param the pointer to the received translation job response.
                     */
                    inline void set_trans_job_resp(trans_job_resp_in * trans_resp) {
                        recursive_guard guard(m_g_lock);

                        ASSERT_SANITY_THROW((m_phase = phase::RESPONSE_PHASE),
                                string("Improper job phase: ") + to_string(m_phase));

                        //Store the translation job reponse
                        m_trans_resp = trans_resp;

                        //Now we are in the reply phase, the reply is to be sent to the client
                        m_phase = phase::REPLY_PHASE;
                    }

                    /**
                     * Allows to cancel the given translation job by telling all the translation tasks to stop.
                     * Calling this method indicates that the job is canceled due to the client disconnect
                     */
                    inline void cancel() {
                        recursive_guard guard(m_g_lock);

                        //Depending on the phase
                        switch (m_phase) {
                                //Waiting when the reply is sent to the client - are in the outgoing tasks pool
                            case phase::REPLY_PHASE:
                                //Waiting when the request is sent to the translator - are in the incoming tasks pool
                            case phase::REQUEST_PHASE:
                                //Waiting for the translation response, will be triggered by the translator
                            case phase::RESPONSE_PHASE:
                            {
                                //Just mark the state as canceled the rest will be done by execute
                                m_state = state::CANCELED_STATE;
                                break;
                            }
                                //The reply is already sent to the client, nothing to be done
                            case phase::DONE_PHASE:
                                //The state is undefined, this should not be happening
                            case phase::UNDEFINED_PHASE:
                            default:
                            {
                                //Log an error as we are in an improper state
                                LOG_DEBUG << "ERROR: Wrong phase: " << *this << " is canceled!" << END_LOG;
                                break;
                            }
                        }
                    }

                    /**
                     * Allows to cancel the given job. Calling this method indicates
                     * that the job is canceled due to the translator's adapter disconnect
                     * This method is synchronized.
                     */
                    inline void fail() {
                        recursive_guard guard(m_g_lock);

                        //Depending on the phase
                        switch (m_phase) {
                                //Waiting when the reply is sent to the client - are in the outgoing tasks pool
                            case phase::REPLY_PHASE:
                            {
                                //Nothing to be done as we already know what to send to the client
                                break;
                            }
                                //Waiting for the translation response
                            case phase::RESPONSE_PHASE:
                            {
                                //Check if the job is not canceled. If it is then it should stay canceled
                                //As in this case we already know that there will be no client to reply.
                                if (m_state == state::CANCELED_STATE) {
                                    //Notify the problem, do not change the state or error message
                                    report_communication_error(m_state, m_err_msg);
                                } else {
                                    //Notify the problem, the translation has failed
                                    report_communication_error(state::FAILED_STATE,
                                            "The translation server has dropped connection!");
                                }
                                break;
                            }
                                //Waiting when the request is sent to the translator,
                                //can't not fail here, the adapter is not yet chosen.
                            case phase::REQUEST_PHASE:
                                //The reply is already sent to the client, nothing to be done
                            case phase::DONE_PHASE:
                                //The state is undefined, this should not be happening
                            case phase::UNDEFINED_PHASE:
                            default:
                            {
                                //Log an error as we are in an improper state
                                LOG_DEBUG << "ERROR: Wrong phase: " << *this << " is canceled!" << END_LOG;
                                break;
                            }
                        }
                    }

                    /**
                     * Allows to wait until the job is finished, this
                     * includes the notification of the job pool.
                     * This method is synchronized.
                     */
                    inline void synch_job_finished() {
                        recursive_guard guard(m_f_lock);
                    }

                    /**
                     * Performs balancer job actions depending on the internal job state
                     * The execute is to be called in two phases:
                     * phase::REQUEST_PHASE - this is when we need to send the request to the translator
                     * phase::REPLY_PHASE - this is when the translator's response is received and we need to send it to the client.
                     */
                    void execute() {
                        recursive_guard guard(m_g_lock);

                        switch (m_phase) {
                            case phase::REQUEST_PHASE:
                            {
                                //Send the request to the translator
                                send_request();
                                break;
                            }
                            case phase::REPLY_PHASE:
                            {
                                //Send the reply to the client
                                send_reply();
                                break;
                            }
                            default:
                            {
                                //This must not be happening it is an internal error
                                LOG_ERROR << "Executing the balancer job in phase: " << m_phase << END_LOG;
                            }
                        }
                    }

                protected:

                    /**
                     * Allows to report the request sending error
                     * @param state_value the new state value
                     * @param err_msg the error message
                     */
                    inline void report_communication_error(const state state_value, const string err_msg) {
                        //This must not be happening it is an internal error
                        LOG_DEBUG << "ERROR: " << err_msg << END_LOG;

                        //The job has been sent, change the phase
                        m_phase = phase::REPLY_PHASE;
                        //Change the state to the given one
                        m_state = state_value;
                        //Store the error message
                        m_err_msg = err_msg;
                        //Register an error response
                        m_notify_err_func(this);
                    }

                    /**
                     * Is called when it is time to send the request to the translator.
                     * There is the situations to consider: 
                     * 1. The job is already canceled due to the client disconnect.
                     * 2. The translator's adapter is not available.
                     * 3. The request sending failed.
                     * 4. Everything went fine.
                     * This method is not synchronized. It must be called from a thread safe context.
                     */
                    inline void send_request() {
                        //Register the job by the given job id as awaiting response
                        m_register_wait_func(this);

                        switch (m_state) {
                            case state::ACTIVE_STATE:
                            {
                                //Get the translator's adapter
                                translator_adapter * adapter = m_choose_adapt_func(m_trans_req);

                                //Check if the adapter is present
                                if (adapter != NULL) {
                                    //Store the adapter uid
                                    m_adapter_uid = adapter->get_uid();
                                    //Prepare the request with the new job id
                                    m_trans_req->set_job_id(m_bal_job_id);
                                    //Attempt sending the request through the adapter
                                    try {
                                        adapter->send(m_trans_req->get_message());
                                        //The job has been sent, change the phase
                                        m_phase = phase::RESPONSE_PHASE;
                                    } catch (std::exception & ex) {
                                        //If the sending is failed, register an error response
                                        report_communication_error(state::FAILED_STATE, ex.what());
                                    }
                                } else {
                                    //If the adapter is not present, register an error response
                                    report_communication_error(state::FAILED_STATE,
                                            "There are no online servers to perform your translation request!");
                                }
                                break;
                            }
                            case state::CANCELED_STATE:
                            {
                                //The client session was terminated so the request does not need to be sent
                                report_communication_error(state::CANCELED_STATE,
                                        "The client session was terminated, canceling the request!");
                                break;
                            }
                            default:
                            {
                                report_communication_error(state::FAILED_STATE,
                                        string("Internal error while sending request, state: ") + to_string(m_state));
                            }
                        }
                    }

                    /**
                     * Allows to prepare an error reply to the client. The response is filled 
                     * in with the original text and the job id but with an error status.
                     * @param resp a reference to the response to be filled in
                     */
                    inline void prepare_error_reply(trans_job_resp_out & resp) {
                        resp.set_job_id(m_job_id);
                        resp.set_status(status_code::RESULT_ERROR, m_err_msg);
                        resp.begin_sent_data_arr();
                        //Get the text that had to be translated
                        const Value & source_text = m_trans_req->get_source_text();
                        //Get the sentence data writer
                        trans_sent_data_out & sent_data = resp.get_sent_data_writer();
                        //Copy the input sentences but, set the canceled status
                        for (auto iter = source_text.Begin(); iter != source_text.End(); ++iter) {
                            //Begin the sentence data
                            sent_data.begin_sent_data_ent();
                            //Set the target sentence
                            sent_data.set_trans_text(iter->GetString());
                            //Set the sentence status
                            sent_data.set_status(status_code::RESULT_ERROR, "Failed to translate");
                            //End the sentence data section
                            sent_data.end_sent_data_ent();
                        }
                        resp.end_sent_data_arr();
                    }

                    /**
                     * Is called when it is time to send the request to the translator.
                     * There is the situations to consider: 
                     * 1. The job is canceled by the client session disconnect
                     * 2. The job is canceled by the server session disconnect
                     * 3. Everything is fine
                     * This method is partially synchronized. It must be called from a thread safe context.
                     * The only synchronization available is with the synch_job_finished method.
                     */
                    inline void send_reply() {
                        switch (m_state) {
                            case state::ACTIVE_STATE:
                            {
                                //Change the job id in the response to the stored - original - one
                                m_trans_resp->set_job_id(m_job_id);
                                //Perform the sanity check
                                ASSERT_SANITY_THROW((m_trans_resp == NULL), "The translation response is NULL!");
                                //Send the response to the client through the sender function
                                m_resp_send_func(m_session_id, *m_trans_resp);
                                break;
                            }
                            case state::CANCELED_STATE:
                            {
                                //Do nothing, the client was disconnected, just log an issue 
                                LOG_DEBUG << "Could not send the job " << *this
                                        << " back, the client is disconnected!" << END_LOG;
                                break;
                            }
                            case state::FAILED_STATE:
                            {
                                //Create a response 
                                trans_job_resp_out resp;
                                //Fill it in with data
                                prepare_error_reply(resp);
                                //Send to the client
                                m_resp_send_func(m_session_id, resp);
                                break;
                            }
                            default:
                            {
                                //This must not be happening it is an internal error
                                LOG_ERROR << "Sending the job REPLY in state: " << m_state << END_LOG;
                            }
                        }

                        //The job has been sent, change the phase
                        m_phase = phase::DONE_PHASE;

                        //Notify that the job is now finished.
                        {
                            recursive_guard guard(m_f_lock);

                            //Notify that this job id done
                            m_notify_job_done_func(this);
                        }
                    }

                private:
                    //Stores the static instance of the id manager
                    static id_manager<job_id_type> m_id_mgr;

                    //Stores the translation client session id
                    const session_id_type m_session_id;

                    //Stores the original job id of the request
                    const job_id_type m_job_id;

                    //Stores the pointer to the incoming translation job request, not NULL
                    trans_job_req_in * m_trans_req;

                    //Stores the pointer to the incoming translation job response, NULL until it is received
                    trans_job_resp_in * m_trans_resp;

                    //The done job notifier
                    done_job_notifier m_notify_job_done_func;

                    //ToDo: Make the next functions static

                    //Stores the reference to the function for choosing the appropriate translation adapter
                    const adapter_chooser & m_choose_adapt_func;

                    //Stores the reference to the function for registering that the job is awaiting a response
                    const job_notifier & m_register_wait_func;

                    //Stores the reference to the function for notifying about the error response
                    const job_notifier & m_notify_err_func;

                    //Stores the reference to the function for sending the translation response to the client
                    const session_response_sender & m_resp_send_func;

                    //Stores the balancer job phase
                    phase m_phase;

                    //Stores the balancer job state
                    state m_state;

                    //Stores the error message
                    string m_err_msg;

                    //The global lock needed to guard the job state change and its execution
                    recursive_mutex m_g_lock;

                    //The final lock needed to guard the job ready notification and
                    //waiting for it is finished before the job is deleted.
                    recursive_mutex m_f_lock;

                    //Stores the balancer job id, is initialized once the job is sent
                    const job_id_type m_bal_job_id;

                    //Stores the adapter uid, is initialized after the adapter is retrieved
                    server_uid_type m_adapter_uid;
                };
            }
        }
    }
}

#endif /* BALANCER_JOB_HPP */

