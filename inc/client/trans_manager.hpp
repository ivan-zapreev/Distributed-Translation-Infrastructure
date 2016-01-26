/* 
 * File:   trans_manager.hpp
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
 * Created on January 26, 2016, 1:06 PM
 */

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <cstdlib>

#include "client_config.hpp"
#include "translation_client.hpp"
#include "common/messaging/id_manager.hpp"
#include "common/messaging/trans_job_id.hpp"
#include "common/messaging/trans_job_request.hpp"
#include "common/messaging/trans_job_response.hpp"
#include "common/utils//file/CStyleFileReader.hpp"

using namespace std;

#ifndef TRANS_MANAGER_HPP
#define TRANS_MANAGER_HPP

namespace uva {
    namespace smt {
        namespace decoding {
            namespace client {

                /**
                 * This is the client side translation manager class. It's task
                 * is to get the source text from a file and then split it into
                 * a number of translation jobs that will be sent to the
                 * translation server. The finished translation jobs are collected
                 * and once all of them are finished the resulting text is written
                 * into the output file.
                 */
                class trans_manager {
                public:
                    //Stores the absolute allowed minimum of sentences to be sent by translation request.
                    //Note that if the translation text is smaller then this value is overruled.
                    static constexpr uint64_t MIN_SENTENCES_PER_REQUEST = 5;

                    /**
                     * This is the basic constructor needed to 
                     * @param params the translation client parameters
                     */
                    trans_manager(const client_config & params)
                    : m_params(params),
                    m_client(m_params.m_server, m_params.m_port),
                    m_source_file(params.m_source_file) {
                        //If the input file could not be opened, we through!
                        ASSERT_CONDITION_THROW(!m_source_file.is_open(),
                                string("Could not open the source text file: ") + params.m_source_file);

                        //Log the source file reader info
                        m_source_file.log_reader_type_usage_info();

                        //Check that the minimum is not larger than the maximum
                        ASSERT_CONDITION_THROW((m_params.m_max_sent < m_params.m_min_sent),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") is larger that the maximum (") + to_string(m_params.m_max_sent) + string(")!"));

                        //Check that the minimum is not too small
                        ASSERT_CONDITION_THROW((m_params.m_min_sent < MIN_SENTENCES_PER_REQUEST),
                                string("The minimum number of sentences to be sent (") + to_string(m_params.m_min_sent)
                                + string(") should be larger or equal than") + to_string(MIN_SENTENCES_PER_REQUEST));
                    }

                    /**
                     * The basic destructor class
                     */
                    virtual ~trans_manager() {
                        //Close the source file
                        m_source_file.close();
                    }

                    /**
                     * Allows to start the translation process
                     */
                    void start() {
                        if (m_client.connect()) {
                            //ToDo: Run the translation job sending thread
                        } else {
                            THROW_EXCEPTION(string("Could not open the connection to: ") + m_client.get_uri());
                        }
                    }

                    /**
                     * Allows to wait until the translations are done
                     */
                    void wait() {
                        //ToDo: Wait until we are notified that the translation are done
                    }

                    void stop() {
                        //ToDo: Stop the translation job sending thread
                        //ToDo: Close the connection
                        m_client.disconnect();
                        //ToDo: Write the translations we have so far into the file
                    };

                protected:

                    /**
                     * This function shall be run in a separate thread and send a number of translation job requests to the server.
                     * @param params the client parameters storing, e.g., the input text file name
                     */
                    void send_translation_jobs() {
                        //Declare the boolean to store the stop condition
                        bool is_finished = false;
                        //Declare the variable to store the sentence line
                        TextPieceReader line;

                        LOG_DEBUG << "Reading text from the source file: " << END_LOG;
                        while (!is_finished) {
                            //Get the number of sentences to send in the next request
                            const uint64_t num_to_sent = get_num_of_sentences();
                            //Stores the number of read sentences
                            uint64_t num_read = 0;
                            //text to send for translation
                            string source_text;

                            while (m_source_file.get_first_line(line) && (num_read < num_to_sent)) {
                                LOG_DEBUG << "Read line: '" << line.str() << "'" << END_LOG;

                                //Append the new line to the text to be sent
                                source_text += line.str() + "\n";

                                //Increment the number of read sentences
                                ++num_read;
                            }

                            //If there were lines read then do translation
                            if (num_read != 0) {
                                //Get the new job id
                                const job_id_type job_id = m_id_mgr.get_next_id();

                                //Create the translation job request 
                                trans_job_request request(job_id, m_params.m_source_lang, source_text, m_params.m_target_lang);

                                try {
                                    //Send the translation job request
                                    m_client.send(request);
                                    
                                    //ToDo: Add the translation job request into our administration
                                } catch (Exception e) {
                                    //ToDo: Mark the translation job request as failed in the administration
                                }
                            } else {
                                break;
                            }
                        }

                        //ToDo: Notify that all the translation jobs have been sent!
                    }

                    /**
                     * Allows to compute the number of sentences to send with the next request
                     * @return the number of sentences to send with the next request
                     */
                    uint64_t get_num_of_sentences() {
                        if (m_params.m_min_sent != m_params.m_max_sent) {
                            return m_params.m_min_sent + rand() % (m_params.m_max_sent - m_params.m_min_sent) + 1;
                        } else {
                            return m_params.m_min_sent;
                        }
                    }

                private:
                    //Stores the static instance of the id manager
                    static id_manager<job_id_type> m_id_mgr;

                    //Stores a reference to the translation client parameters
                    const client_config & m_params;

                    //Stores the translation client
                    translation_client m_client;


                    //Stores the source text
                    CStyleFileReader m_source_file;

                    //Stores the mapping from the translation job request id to
                    //the resulting translation job result, if already received.
                    //The translation jobs without a reply are mapped to NULL
                    unordered_map<job_id_type, trans_job_response *> m_jobs;
                };

                id_manager<job_id_type> trans_manager::m_id_mgr(job_id::MINIMUM_JOB_ID);
            }
        }
    }
}

#endif /* TRANS_MANAGER_HPP */

