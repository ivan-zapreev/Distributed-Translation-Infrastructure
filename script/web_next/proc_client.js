/**
 * Allows to create a new processor server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the processor server client module
 */
function create_proc_client(logger_mdl, language_mdl, url_input, url,
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
     * Allows to send the processor job requests
     * @param md5 {String} the md5 sum of the text to be used as a job token
     * @param text {String} the text to be sent for processing
     * @param lang {String} the lang the text is written in
     */
    function send_requests(md5, text, lang) {
        //ToDo: Implement
    }
    
    //Define some exported elements
    module.logger_mdl = logger_mdl;
    module.language_mdl = language_mdl;
    module.process_stop_fn = process_stop_fn;
    module.send_requests_fn = send_requests;
    
    return module;
}