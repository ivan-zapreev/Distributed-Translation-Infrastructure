/* 
 * File:   translation_server.hpp
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
 * Created on January 14, 2016, 2:39 PM
 */

#ifndef TRANSLATION_SERVER_HPP
#define TRANSLATION_SERVER_HPP

#include "common/messaging//websocket_server.hpp"
#include "common/utils/cmd/cmd_line_client.hpp"

#include "server/translation_manager.hpp"
#include "server/server_parameters.hpp"

using namespace std;
using namespace uva::utils::cmd;
using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This is the translation server class implementing the functionality of
                 * receiving the client connections and doing translation jobs for them.
                 * 
                 * @param TLS_CLASS the TLS class which defines the server type and mode
                 */
                template<typename TLS_CLASS>
                class translation_server :
                public websocket_server<TLS_CLASS>,
                public cmd_line_client {
                public:

                    /**
                     * The basic constructor
                     * @param params the server parameters
                     */
                    translation_server(const server_parameters &params)
                    : websocket_server<TLS_CLASS>(params),
                    m_manager(params.m_num_threads), m_params(params) {
                        //Initialize the supported languages and store the response for future use
                        supp_lang_resp_out supp_lang_resp;
                        //Add the supported languages
                        supp_lang_resp.start_supp_lang_obj();
                        supp_lang_resp.start_source_lang_arr(params.m_source_lang);
                        supp_lang_resp.add_target_lang(params.m_target_lang);
                        supp_lang_resp.end_source_lang_arr();
                        supp_lang_resp.end_supp_lang_obj();

                        m_supp_lang_resp = supp_lang_resp.serialize();

                        //Set the reply sending function to the translation manager
                        m_manager.set_response_sender(bind(&translation_server::send_response, this, _1, _2));
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void report_run_time_info() override {
                        m_manager.report_run_time_info();
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void set_num_threads(const int32_t num_threads) override {
                        m_manager.set_num_threads(num_threads);
                    }

                    /**
                     * @see cmd_line_client
                     */
                    virtual void request_stop() override {
                        this->stop();
                    }

                protected:

                    /**
                     * @see websocket_server
                     */
                    virtual void after_stop_listening() override {
                        LOG_USAGE << "Stopping the translation manager." << END_LOG;
                        //Stop the translation manager, this should cancel all the unfinished translation tasks
                        m_manager.stop();
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void open_session(websocketpp::connection_hdl hdl) override {
                        m_manager.open_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void close_session(websocketpp::connection_hdl hdl) override {
                        m_manager.close_session(hdl);
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void language_request(
                            websocketpp::connection_hdl hdl, supp_lang_req_in * msg) override {
                        //Send the response supported languages response
                        websocket_server<TLS_CLASS>::send_response(hdl, m_supp_lang_resp);

                        //Destroy the message
                        delete msg;
                    }

                    /**
                     * @see websocket_server
                     */
                    virtual void translation_request(
                            websocketpp::connection_hdl hdl, trans_job_req_in * msg) override {
                        //Declare the job id for the case of needed error reporting
                        job_id_type job_id_val = job_id::UNDEFINED_JOB_ID;
                        try {
                            //Store the job id in case of an error
                            job_id_val = msg->get_job_id();

                            //Check the source and target languages
                            check_source_target_languages(msg->get_source_lang(), msg->get_target_lang());

                            //Schedule a translation job
                            m_manager.translate(hdl, *msg);
                        } catch (std::exception & ex) {
                            //Get the error message
                            string error_msg = ex.what();

                            //Locally report error
                            LOG_DEBUG << "job " << job_id_val << ": " << error_msg << END_LOG;

                            //Create the reply message, with or without job id
                            trans_job_resp_out response(job_id_val, status_code::RESULT_ERROR, error_msg);

                            //Send the response
                            websocket_server<TLS_CLASS>::send_response(hdl, response.serialize());
                        }

                        //Delete the request message
                        delete msg;
                    }

                    /**
                     * Allows to check that the source and target languages are proper,
                     * as the ones that this server instance supports.
                     * @param source_lang the source language string, from the job request.
                     * @param target_lang the target language string, from the job request.
                     * @throws uva_exception in case the source or target language do not match.
                     */
                    void check_source_target_languages(string source_lang, string target_lang) {
                        //Check for an exact match
                        if (m_params.m_source_lang != source_lang) {
                            LOG_DEBUG1 << m_params.m_source_lang << " != " << source_lang << END_LOG;
                            //If no exact match then lowercase
                            (void) to_lower(source_lang);
                            //Check for a match again
                            if (m_params.m_source_lang_lower != source_lang) {
                                LOG_DEBUG1 << m_params.m_source_lang_lower << " != " << source_lang << END_LOG;
                                //If there is still no match then throw 
                                THROW_EXCEPTION(string("Unsupported source language: '") + source_lang +
                                        string("' the server only supports: '") + m_params.m_source_lang +
                                        string("'"));
                            }
                        }
                        //Check for an exact match
                        if (m_params.m_target_lang != target_lang) {
                            LOG_DEBUG1 << m_params.m_target_lang << " != " << target_lang << END_LOG;
                            //If no exact match then lowercase
                            (void) to_lower(target_lang);
                            //Check for a match again
                            if (m_params.m_target_lang_lower != target_lang) {
                                LOG_DEBUG1 << m_params.m_target_lang_lower << " != " << target_lang << END_LOG;
                                //If there is still no match then throw 
                                THROW_EXCEPTION(string("Unsupported target language: '") + target_lang +
                                        string("' the server only supports: '") + m_params.m_target_lang +
                                        string("'"));
                            }
                        }
                    }

                private:
                    //Stores the translation manager object
                    translation_manager m_manager;
                    //Stores the reference to the server options
                    const server_parameters &m_params;
                    //Stores the serialized supported languages response string
                    string m_supp_lang_resp;
                };

#if IS_TLS_SUPPORT
                //Add the TLS servers as an option in case the TLS support is enabled
                typedef translation_server<server_tls_handshake_old> translation_server_tls_old;
                typedef translation_server<server_tls_handshake_int> translation_server_tls_int;
                typedef translation_server<server_tls_handshake_mod> translation_server_tls_mod;
#endif
                //Add the no-TLS server for when TLS is not enabled or needed
                typedef translation_server<without_tls_handshake> translation_server_no_tls;

            }
        }
    }
}

#endif /* TRANSLATION_SERVER_HPP */

