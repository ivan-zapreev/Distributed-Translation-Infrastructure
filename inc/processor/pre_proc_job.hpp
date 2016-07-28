/* 
 * File:   pre_proc_job.hpp
 * Author: zapreevis
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
 * Created on July 26, 2016, 4:19 PM
 */

#ifndef PRE_PROC_JOB_HPP
#define PRE_PROC_JOB_HPP

#include <stdio.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sstream>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"

#include "processor/processor_job.hpp"
#include "processor/messaging/proc_req_in.hpp"
#include "processor_parameters.hpp"

using namespace std;
using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::threads;
using namespace uva::smt::bpbd::processor::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                /**
                 * This is the pre-processor job:
                 * Responsibilities:
                 *    - Stores the pre-processor job specific data
                 *    - Execute the pre-processor job
                 *    - Gather the results of the job
                 *    - Send the results back to the client 
                 */
                class pre_proc_job : public processor_job {
                public:

                    /**
                     * The basic constructor
                     * @param config the language configuration, might be undefined.
                     * @param session_id the id of the session from which the translation request is received
                     * @param req the pointer to the pre-processor request, not NULL
                     */
                    pre_proc_job(const language_config & config,
                            const session_id_type session_id, proc_req_in * req)
                    : processor_job(config, session_id, req) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~pre_proc_job() {
                    }

                    /**
                     * @see processor_job
                     */
                    virtual void execute() override {
                        //Check if the provided language configuration is defined
                        const language_config & conf = this->get_lang_config();
                        if (conf.is_defined()) {
                            //Define the job uid string
                            string job_uid_str;
                            //Create the file name for the text we need to pre-process.
                            const string file_name = this->template get_text_file_name<true, true>(job_uid_str);

                            try {
                                //Save the file to the disk
                                this->store_text_to_file(file_name);

                                //Get the string needed to call the pre-processor script
                                const string call_str = conf.get_call_string(job_uid_str, this->get_language());

                                //Call the pre-processor script
                                stringstream output;
                                if (call_pre_processor(call_str, output)) {
                                    //ToDo: Send the responses to the client. The
                                    //output must contain the source language.
                                } else {
                                    //ToDo: Report an error to the client. The
                                    //output must contain the error message.
                                }
                            } catch (std::exception & ex) {
                                LOG_ERROR << "Could not pre-process: " << file_name << ", language: " <<
                                        this->get_language() << ", error: " << ex.what() << END_LOG;
                                //ToDo: Report an error to the client.
                            }
                        } else {
                            LOG_DEBUG << "The language configuration is empty, meaning " <<
                                    "that the language '" << this->get_language() << "' is not " <<
                                    "supported, and there is not default pre-procesor!" << END_LOG;
                            //ToDo: Report an error to the client.
                        }
                    }

                    /**
                     * Calls the pre-processor script and reads from its output, the script is expected to do one of:
                     * 1. Execute normally and then to write the source language name into its output
                     * 2. Fail to execute the job - return an error code - and then to write an error into the output
                     * @param call_str the call string
                     * @param output the output of the script
                     * @return true if the pre-processor finished the job without errors, otherwise false
                     */
                    inline bool call_pre_processor(const string &call_str, stringstream & output) {
                        FILE *fp = popen(call_str.c_str(), "r");
                        if (fp != NULL) {
                            //The buffer itself
                            char buffer[1024];
                            //Read from the pipeline - we shall get the resulting language or an error message
                            while (fgets(buffer, sizeof (buffer), fp) != NULL) {
                                output << buffer;
                            }

                            //Wait until the process finishes and analyze its status
                            int status = pclose(fp);
                            if (status == -1) {
                                //Error the process status is not possible to retrieve!
                                THROW_EXCEPTION(string("Could not get the script ") +
                                        call_str + (" execution status!"));
                            } else {
                                if (WIFEXITED(status) != 0) {
                                    //The script terminated normally, check if there were errors
                                    return ((WEXITSTATUS(status) == 0) || (WEXITSTATUS(status) == EXIT_SUCCESS));
                                } else {
                                    THROW_EXCEPTION(string("The pre-processor script ") +
                                            call_str + string(" terminated abnormally!"));
                                }
                            }
                        } else {
                            THROW_EXCEPTION(string("Failed to call the pre-processor script: ") + call_str);
                        }
                    }

                    /**
                     * @see processor_job
                     */
                    virtual void synch_job_finished() override {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                    /**
                     * @see processor_job
                     */
                    virtual void cancel() override {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                };
            }
        }
    }
}

#endif /* PRE_PROC_JOB_HPP */

