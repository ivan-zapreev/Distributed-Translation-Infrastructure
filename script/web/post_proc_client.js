/*
 *  File:   post_proc_client.js
 *  Author: Dr. Ivan S. Zapreev
 *  Visit my Linked-in profile:
 *       <https://nl.linkedin.com/in/zapreevis>
 *  Visit my GitHub:
 *       <https://github.com/ivan-zapreev>
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  Created on November 14, 2016, 11:07 AM
 */

/**
 * Allows to create a new post-processor server client
 * @param module {Object} the processor client module
 * @return the post-processor server client module
 */
function create_post_proc_client(module) {
    "use strict";

    /**
     * This function is called when the translation process needs to begin
     * @patam target_text {String} the target text to post-process
     * @param job_token {String} the job token to be used when talking to the post-processor
     */
    function process(target_text, job_token) {
        window.console.log("Starting post-processing text: " + target_text);
        window.console.log("The post-processing text job_token is: " + job_token);

        //Pre-declare variables
        var target_lang, job_req_base;
        
        //Get the source text language
        target_lang = module.common_mdl.lang_mdl.get_sel_target_lang_fn();
        
        //Check if the processor is connected or not
        if (module.is_connected_fn()) {
            //If connected then get the target language and start sending the data
            window.console.log("The post-processing module is connected!");
            
            //The job request base
            job_req_base = module.create_jr_base_fn(
                module.MSG_TYPE_ENUM.MESSAGE_POST_PROC_JOB_REQ,
                job_token,
                target_lang
            );
            
            //Send the processor request in chunks
            module.send_requests_fn(job_req_base, target_text);
        } else {
            window.console.log("The pre-processing module is disconnected!");
            //The job is finished, no need to post-process
            module.common_mdl.process_stop_fn(false, "");
            //Note: We do not need to output any information as it must
            //have been done by the previous modules in the queue.
        }
    }
    
    /**
     * Will be called once the post-processor job is finished.
     * @param result_text {String} the resulting text
     * @param job_token {String} the resulting job token
     * @param res_language {String} the resulting language
     */
    function notify_processor_ready(target_text, job_token, target_lang) {
        //The job is finished, nothing to be done, the output is done by the base module.
        module.common_mdl.process_stop_fn(false, "");
    }
    
    //Store the post-processing function pointer
    module.process_fn = process;
    //Set the ready notifier function
    module.notify_processor_ready_fn = notify_processor_ready;
    
    return module;
}
