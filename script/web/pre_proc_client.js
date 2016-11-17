/*
 *  File:   pre_proc_client.js
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
 * Allows to create a new pre-processor server client
 * @param module {Object} the processor client module
 * @param trans_serv_mdl {Object} the translation module
 * @return the pre-processor server client module
 */
function create_pre_proc_client(module, trans_serv_mdl) {
    "use strict";

    /**
     * This function is called when the translation process needs to begin
     * @patam source_md5 {String} the source text md5 sum to be used as a job token
     * @param source_text {String} the source text to work with
     */
    function process(source_text, source_md5) {
        window.console.log("Starting pre-processing text: " + source_text);

        //Pre-declare variables
        var source_lang, job_req_base;
        
        //Get the source text language
        source_lang = module.common_mdl.lang_mdl.get_sel_source_lang_fn();
        
        //Check if the processor is connected or not
        if (module.is_connected_fn()) {
            //If connected then get the source language and start sending the data
            window.console.log("The pre-processing module is connected!");
            
            //The job request base
            job_req_base = module.create_jr_base_fn(
                module.MSG_TYPE_ENUM.MESSAGE_PRE_PROC_JOB_REQ,
                source_md5,
                source_lang
            );
            
            //Send the processor request in chunks
            module.send_requests_fn(job_req_base, source_text);
        } else {
            window.console.log("The pre-processing module is disconnected!");
            //Check if the language is selected, i.e. also not autodetect.
            if (module.common_mdl.lang_mdl.is_source_lang_sel_fn()) {
                window.console.log("The source language is selected to: " + source_lang);
                trans_serv_mdl.process_fn(source_text, source_md5, source_lang);
            } else {
                module.common_mdl.process_stop_fn(true, "The pre-processor is not connected, the language detection is not possible!");
            }
        }
    }
    
    /**
     * Will be called once the pre-processor job is finished.
     * @param result_text {String} the resulting text
     * @param job_token {String} the resulting job token
     * @param res_language {String} the resulting language
     */
    function notify_processor_ready(source_text, job_token, source_lang) {
        if (module.common_mdl.lang_mdl.set_detected_source_lang_fn(source_lang)) {
            window.console.log("The source language: " + source_lang + " is set.");
            
            //Call the translator, the output is done by the base module.
            trans_serv_mdl.process_fn(source_text, job_token, source_lang);
        } else {
            //Could not set the source/target languages
            module.common_mdl.visualize_sc_fn(job_token,
                                              module.common_mdl.STATUS_CODE_ENUM.RESULT_ERROR,
                                              "The detected source language: " + source_lang + " is not supported!");
            module.common_mdl.process_stop_fn(false, "");
        }
    }
    
    //Store the pre-processing function pointer
    module.process_fn = process;
    //Set the ready notifier function
    module.notify_processor_ready_fn = notify_processor_ready;
    
    return module;
}
