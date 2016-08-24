//The maximum processor message text size is characters. This limits
//the number of characters to send per processor request.
var MESSAGE_MAX_CHAR_LEN = 10 * 1024;

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
    
    var is_working, expected_responces, received_responces,
        job_responces, job_token, resp_language, module;
    
    //Default initialize the variables
    is_working = false;
    
    /**
     * Allows to re-set the client constants
     * @param is_wk {Boolean} true if we are doing processing, default is false
     * @param exp_resp {Number} the number of expected responses default is 0
     */
    function re_set_client(is_wk, exp_resp) {
        is_working = is_wk || false;
        expected_responces = exp_resp || 0;
        received_responces = 0;
        job_responces = [];
        job_token = "";
        resp_language = "";
        
        window.console.log("Re-setting the internal variables" +
                           ", is_working: " + is_working +
                           ", expected_responces: " + expected_responces);
    }
    
    //Re-set the local variables
    re_set_client();
    
    /**
     * This function is called when a message is received from the processor server
     * @param resp_obj the received json message 
     */
    function on_message(resp_obj) {
        window.console.log("A processor server message is received");
        
        //Check of the message type
        if ((resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_PRE_PROC_JOB_RESP)
                || (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_POST_PROC_JOB_RESP)) {
            //Check if the job token has changed
            if (job_token !== resp_obj.job_token) {
                //Store the new job token
                job_token = resp_obj.job_token;
                //Store the number of expected responses
                expected_responces = resp_obj.num_chs;
                //Store the language
                resp_language = resp_obj.lang;
                //Log the data
                window.console.log("Got a new processor response job token: " + job_token +
                                   ", expected_responces: " + expected_responces +
                                   ", resp_language: " + resp_language);
            }
            
            //Place the token into the response array
            job_responces[resp_obj.ch_idx] = resp_obj;
            
            //Increase the number of received responses
            received_responces += 1;
            
            //Update the progress bar
            module.set_response_pb_fn(received_responces, expected_responces);
            
            //Check if all the responces have been received, then process
            if (received_responces === expected_responces) {
                module.logger_mdl.success("Received all of the " + expected_responces +
                                          " processor responses, language: " + resp_language);
                
                //ToDo: Combine the responces into on text and call the subsequent
            }
        } else {
            module.logger_mdl.danger("An unsupported server message type: " + resp_obj.msg_type);
        }
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

        //Re-set the client
        re_set_client();

        window.console.log("The connection to: '" + module.url + "'has failed!");
    }
    
    //Instantiate the module base
    module = create_ws_client_fn(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 null, on_message, on_close, escape_html_fn,
                                 request_progress_bar, response_progress_bar);
    
    /**
     * Allows to create a processor job request base object
     * @param msg_type {Number} the message type
     * @param job_token {String} the job token
     * @param lang {String} the language
     */
    function create_job_req_base(msg_type, job_token, lang) {
        //Log the data
        window.console.log("Creating a new processor job type: " + msg_type +
                           ", job token: " + job_token + ", language: " + lang);
        
        return {prot_ver : module.PROTOCOL_VERSION,
                msg_type : msg_type, job_token : job_token,
                lang : lang};
    }
    
    /**
     * Allows to send the processor job requests
     * @param job_req_base {Object} the job request object
     *        base to be used for the job requests when
     *        sending the text chunks.
     * @param text {String} the text to be sent for processing
     */
    function send_requests(job_req_base, text) {
        //Declare variables
        var ch_idx, num_chunks, job_req, log_ch_idx;
        
        //Compute the number of chunks we will split the text into
        num_chunks = Math.ceil(text.length / MESSAGE_MAX_CHAR_LEN);
        
        window.console.log("Processor text length: " + text.length +
                           " chars, max chars per chunk: " + MESSAGE_MAX_CHAR_LEN +
                           ", num chunks: " + num_chunks);
        
        //Set the number of chunks into the job request base
        job_req_base.num_chs = num_chunks;
        
        //Re-set the client
        re_set_client(true, num_chunks);

        //Re-initialize the progress bars
        module.init_req_resp_pb_fn();
        
        //Send the chunks
        for (ch_idx = 0; ch_idx < num_chunks; ch_idx += 1) {
            //Fill in the chunk index and the text to send
            job_req_base.ch_idx = ch_idx;
            job_req_base.text = text.substr(ch_idx * MESSAGE_MAX_CHAR_LEN, MESSAGE_MAX_CHAR_LEN);
            //Send the request 
            module.send_request_fn(job_req_base);
            //Perform logging and progress bar update
            log_ch_idx = (ch_idx + 1);
            window.console.log("Sending processor request chunk: " + log_ch_idx +
                              ", text length: " + job_req_base.text.length);
            module.set_request_pb_fn(log_ch_idx, num_chunks);
        }
        
        //Make the response progress bar note visible
        module.set_response_pb_fn(0, num_chunks);
        
        //Logthe success message
        module.logger_mdl.success("Sent out " + num_chunks + " processor requests");
    }
    
    //Define some exported elements
    module.logger_mdl = logger_mdl;
    module.language_mdl = language_mdl;
    module.process_stop_fn = process_stop_fn;
    module.send_requests_fn = send_requests;
    module.create_jr_base_fn = create_job_req_base;
    
    return module;
}