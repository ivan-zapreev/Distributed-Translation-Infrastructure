//The maximum processor message text size is characters. This limits
//the number of characters to send per processor request.
var MESSAGE_MAX_CHAR_LEN = 10 * 1024;

/**
 * Allows to create a new processor server client
 * @param common_mdl {Object} the common module
 * @param url_input {Object} the url input dom object
 * @param url {String} the server url to work with
 * @param ...
 * @return the processor server client module
 */
function create_proc_client(common_mdl, url_input, url, server_cs_img,
                            server_cs_bage, request_pb, response_pb,
                            pre_post_txt) {
    "use strict";
    
    var is_working, num_exp_resp, num_act_resp,
        job_responces, job_token, resp_language,
        result_text, par_array, target_html, module;
    
    //Default initialize the variables
    is_working = false;
    
    /**
     * Allows to re-set the client constants
     * @param is_wk {Boolean} true if we are doing processing, default is false
     * @param exp_resp {Number} the number of expected responses default is 0
     */
    function re_set_client(is_wk, exp_resp) {
        is_working = is_wk || false;
        num_exp_resp = exp_resp || 0;
        num_act_resp = 0;
        job_responces = [];
        job_token = "";
        resp_language = "";
        result_text = "";
        par_array = [];
        target_html = "";
        
        window.console.log("Re-setting the internal variables" +
                           ", is_working: " + is_working +
                           ", num_exp_resp: " + num_exp_resp);
    }
    
    //Re-set the local variables
    re_set_client();
    
    /**
     * This function allows to visualize the resulting text into the to_text_span
     * @param result_text {String} the text to visualize
     */
    function visualize_and_proceed(par_array, idx) {
        //Add the paragraph as an html span
        target_html += "<span class='target_sent_tag' title='' data-original-title='" +
            pre_post_txt + "-processed text' data-toggle='tooltip' data-placement='auto'>" + common_mdl.escape_html_fn(par_array[idx]) + "</span>";
        
        //Check if this is the last element
        if (idx === (par_array.length - 1)) {
            //Set the data into the html
            common_mdl.to_text_span.html(target_html);
            //Enable tool-tipls
            window.$('[data-toggle="tooltip"]').tooltip();

            //Log the success
            common_mdl.logger_mdl.success("Processed " + job_responces.length + " " +
                                          pre_post_txt + "-processor responses.");
            
            //Set the working flag to false
            is_working = false;
            
            //Call the next module
            module.notify_processor_ready_fn(result_text, job_token, resp_language);

            //Now we have finished all the work, can get to 100%
            module.set_response_pb_fn(num_exp_resp, num_exp_resp);
            
            //Forget the result text and html
            result_text = "";
            target_html = "";
        }
    }

    /**
     * Allows to fill in the single response data into the target data span
     * @param responses {Object} the responses object
     * @param idx {Object} the index of the current response
     */
    function process_response_data(responses, idx) {
        //Decalare local variables
        var response;
        
        window.console.log("Processing " + pre_post_txt + "-processor " +
                           "response number: " + idx);
        
        //Get the current reponse
        response = responses[idx];
        
        //Only visualize the results if the target data is present
        if (response.hasOwnProperty('text')) {
            //Append the result text
            result_text += response.text;
        } else {
            window.console.error("The text field is not present in the " +
                                 pre_post_txt + "-processor response!");
        }
        
        //In case this is the last response that has to be processed
        if (idx === (responses.length - 1)) {
            //Split the text into paragraphs
            par_array = result_text.split('\n');
            
            //Visualize text and proceed to the next phase
            module.process_data_async_fn(par_array, visualize_and_proceed);
        }
    }
    
    /**
     * Allows to process a valid non-error processor job response, that come on time
     * @param resp_obj {Object} a valid non-error processor job response, that comes on time
     */
    function process_response(resp_obj) {
        //Check if the job token has changed
        if (job_token !== resp_obj.job_token) {
            //Store the new job token
            job_token = resp_obj.job_token;
            //Store the number of expected responses
            num_exp_resp = resp_obj.num_chs;
            //Store the language
            resp_language = resp_obj.lang;
            //Log the data
            window.console.log("Got a new " + pre_post_txt + "-processor response " +
                               "job token: " + job_token +
                               ", num_exp_resp: " + num_exp_resp +
                               ", resp_language: " + resp_language);
        }
        
        //Place the token into the response array
        job_responces[resp_obj.ch_idx] = resp_obj;

        //Increase the number of received responses
        num_act_resp += 1;

        //Update the progress bar. Note that the last 10% will be for post processing the responses
        module.set_response_pb_fn(num_act_resp, num_exp_resp * 1.1);

        //Check if all the responces have been received, then process
        if (num_act_resp === num_exp_resp) {
            common_mdl.logger_mdl.success("Received " + num_exp_resp + " " +
                                          pre_post_txt + "-processor" +
                                          " responses, language: " + resp_language);

            //Combine the responces into on text and call the subsequent
            module.process_data_async_fn(job_responces, process_response_data);
        }
    }
    
    /**
     * This function is called when a message is received from the processor server
     * @param resp_obj the received json message 
     */
    function on_message(resp_obj) {
        window.console.log("A processor server message is received");
        
        //Process the message only if we are working
        if (is_working) {
            //Check of the message type
            if ((resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_PRE_PROC_JOB_RESP)
                    || (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_POST_PROC_JOB_RESP)) {
                //Update the visual status
                var job_id = resp_obj.job_token + "/" + resp_obj.ch_idx + "/" + resp_obj.num_chs;
                common_mdl.visualize_sc_fn(job_id, resp_obj.stat_code, resp_obj.stat_msg);
                
                //Check on the error status!
                if (resp_obj.stat_code === common_mdl.STATUS_CODE_ENUM.RESULT_OK) {
                    //If the status is ok - process it
                    process_response(resp_obj);
                } else {
                    //If the status is not OK then report an error and stop
                    common_mdl.process_stop_fn(true, resp_obj.stat_msg);
                    //Re-set the client - as we stop
                    re_set_client();
                }
            } else {
                common_mdl.logger_mdl.danger("An unsupported server message type: " + resp_obj.msg_type);
            }
        } else {
            window.console.warning("Received a processor message when NOT working!");
        }
    }

    /**
     * This function is called when a connection with the processor server is dropped.
     */
    function on_close() {
        if (is_working) {
            common_mdl.logger_mdl.danger("Failed to perform " + pre_post_txt +
                                         "-processing, the server dropped off!", true);
            
            //Report and error and stop
            common_mdl.process_stop_fn(true, "The connection to: '" + module.url + "'has failed!");
        } else {
            common_mdl.logger_mdl.warning("The connection to '" + module.url + "' is closed");
        }

        //Re-set the client
        re_set_client();

        window.console.log("The connection to: '" + module.url + "'has failed!");
    }
    
    //Instantiate the module base
    module = common_mdl.create_ws_client_fn(common_mdl, url_input, url,
                                            server_cs_img, server_cs_bage,
                                            null, on_message, on_close,
                                            request_pb, response_pb);
    
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
        var num_chunks, data;
        
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
        
        /**
         * The function to send the processor requests to the server
         */
        function send_processor_requests(data, ch_idx) {
            var log_ch_idx;
            //Fill in the chunk index and the text to send
            job_req_base.ch_idx = ch_idx;
            job_req_base.text = text.substr(ch_idx * MESSAGE_MAX_CHAR_LEN, MESSAGE_MAX_CHAR_LEN);
            //Send the request 
            module.send_request_fn(job_req_base);
            //Perform logging and progress bar update
            log_ch_idx = (ch_idx + 1);
            window.console.log("Sending processor request chunk: " + log_ch_idx +
                              ", text length: " + job_req_base.text.length);
            module.set_request_pb_fn(log_ch_idx, data.length);
            
            //Report the success after the last one is sent
            if (log_ch_idx === data.length) {
                //Logthe success message
                common_mdl.logger_mdl.success("Sent out " + data.length + " " +
                                              pre_post_txt + "-processor requests");
            }
        }

        //Process the input text, split it into pieces and send to the server
        data = {
            length : num_chunks
        };
        module.process_data_async_fn(data, send_processor_requests);
        
        //Make the response progress bar note visible
        //We add one so that we are not in 100% before
        //everything is done!
        module.set_response_pb_fn(0, num_chunks);
    }
    
    //Define some exported elements
    module.send_requests_fn = send_requests;
    module.create_jr_base_fn = create_job_req_base;
    //Add the processor ready notification function, to be set by a child module.
    module.notify_processor_ready_fn = null;
    
    return module;
}