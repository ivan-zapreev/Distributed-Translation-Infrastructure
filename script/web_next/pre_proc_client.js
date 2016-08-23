/**
 * Allows to create a new pre-processor server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the pre-processor server client module
 */
function create_pre_proc_client(logger_mdl, language_mdl, trans_serv_mdl, url_input, url,
                                 server_cs_img, server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 create_ws_client_fn, escape_html_fn,
                                 request_progress_bar, response_progress_bar,
                                 process_stop_fn) {
    "use strict";
    
    var is_working, expected_responces, received_responces, job_responces, module;

    //Default initialize the variables
    is_working = false;
    expected_responces = 0;
    received_responces = 0;
    job_responces = [];
    
    /**
     * This function is called when a message is received from the processor server
     * @param resp_obj the received json message 
     */
    function on_message(resp_obj) {
        window.console.log("A processor server message is received");
        
        //ToDo: Implement
    }

    /**
     * This function is called when a connection with the processor server is dropped.
     */
    function on_close() {
        if (is_working) {
            logger_mdl.danger("Failed to perform processing the server dropped off!", true);
            //Set the translating flag back to false
            is_working = false;
        } else {
            logger_mdl.warning("The connection to '" + module.url + "' is closed");
        }

        window.console.log("Re-set the internal variables");
        expected_responces = 0;
        received_responces = 0;
        job_responces = [];

        window.console.log("The connection to: '" + module.url + "'has failed!");
    }
    
    //Instantiate the module base
    module = create_ws_client_fn(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 null, on_message, on_close, escape_html_fn,
                                 request_progress_bar, response_progress_bar);

    /**
     * This function is called when the translation process needs to begin
     * @param source_text the source text to work with
     */
    function pre_process(source_text) {
        window.console.log("Starting pre-processing text: " + source_text);

        //Check if the processor is connected or not
        if (module.is_connected_fn()) {
            //If connected then get the source language and start sending the data
            window.console.log("The pre-processing module is connected!");
            
            //ToDo: Implement sending the text in UTF8 chunks
        } else {
            window.console.log("The pre-processing module is disconnected!");
            //Check if the language is selected, i.e. also not autodetect.
            if (language_mdl.is_source_lang_sel_fn()) {
                window.console.log("The source language is selected to: " + language_mdl.get_sel_source_lang_fn());
                trans_serv_mdl.translate_fn(source_text);
            } else {
                process_stop_fn(true, "The pre-processor is not connected, the language detection is not possible!");
            }
        }
    }
    
    //Store the pre-processing function pointer
    module.pre_process_fn = pre_process;
    
    return module;
}
