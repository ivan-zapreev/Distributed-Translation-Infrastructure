//Store the undefined job id value
var UNDEFINED_JOB_ID = 0;
//Stores the minimum translation job id
var MINIMUM_TRANS_JOB_ID = 1;
//The maximum number of sentences per translation job request.
var MAX_NUM_SENTENCES_PER_JOB = 128;

/**
 * Allows to create a new translation server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the translation server client module
 */
function create_trans_client(logger_mdl, lang_mdl, post_serv_mdl, url_input,
                             url, server_cs_img, server_cs_bage, trans_info_cb,
                             needs_new_trans_fn, disable_interface_fn,
                             enable_interface_fn, create_ws_client_fn,
                             escape_html_fn, request_progress_bar,
                             response_progress_bar, process_stop_fn) {
    "use strict";
    
    var SUPPORTED_LANG_REQ, TRAN_JOB_REQ_BASE, prev_job_req_id, is_working,
        sent_trans_req, received_trans_resp, job_responces, job_token,
        result_text, module;

    /**
     * Allows to re-set the client constants
     * @param is_wk {Boolean} true if we are doing processing, default is false
     */
    function re_set_client(is_wk) {
        is_working = is_wk || false;
        sent_trans_req = 0;
        received_trans_resp = 0;
        job_responces = [];
        job_token = "";
        result_text = "";
        prev_job_req_id = MINIMUM_TRANS_JOB_ID;
        
        window.console.log("Re-setting the internal variables" +
                           ", is_working: " + is_working);
    }
        
    //Do default initializations
    re_set_client();
    
    /**
     * Allows to account for a new translation job response
     */
    function count_trans_job_response() {
        //Decrement the number of active translations
        received_trans_resp += 1;
        window.console.log("A new response, remaining #jobs: " + (sent_trans_req - received_trans_resp));

        //Set the current value for the responses, the maximum is equal to the number of requests
        module.set_response_pb_fn(received_trans_resp, sent_trans_req);

        //If all the responses are received
        if (sent_trans_req === received_trans_resp) {
            module.logger_mdl.success("Received all of the " + sent_trans_req +
                                      " translation server responses");

            //ToDo: Process the job responses, give them to post-processor.
            
            //Re-set the client
            re_set_client();
        }
    }

    /**
     * This function is called when a translation response is to be set into the interface
     * Note that the translation response will not be set if it is outdated.
     * @param {String} trans_response the serialized translation job response
     */
    function set_translation(trans_response) {
        //Store the translation response in the array
        job_responces[trans_response.job_id - MINIMUM_TRANS_JOB_ID] = trans_response;

        //Log that we received the response into the console
        logger_mdl.info("Received response for a translation job, id: " + trans_response.job_id);

        //Cound the translation job response
        count_trans_job_response();
    }

    /**
     * This function is called when a connection with the translation server is open.
     */
    function on_open() {
        //Sent the request for the supported languages
        var supp_lang_reg_str = JSON.stringify(SUPPORTED_LANG_REQ);
        window.console.log("Sending the supported languages request: " + supp_lang_reg_str);
        logger_mdl.info("Requesting supported languages from the server!");
        module.ws.send(supp_lang_reg_str);
    }

    /**
     * This function is called when a message is received from the translation server
     * @param resp_obj the received json message 
     */
    function on_message(resp_obj) {
        window.console.log("A translation server message is received");
        
        //Check of the message type
        if (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_RESP) {
            //Set the translation data
            set_translation(resp_obj);
        } else {
            if (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_RESP) {
                //Set the supported languages
                lang_mdl.set_supp_langs_fn(resp_obj);
            } else {
                logger_mdl.danger("An unknown server message type: " + resp_obj.msg_type);
            }
        }
    }

    /**
     * This function is called when a connection with the translation server is dropped.
     */
    function on_close() {
        if (is_working) {
            logger_mdl.danger("Failed to perform translation the server dropped off!", true);
            //Set the translating flag back to false
            is_working = false;
        } else {
            logger_mdl.warning("The connection to '" + module.url + "' is closed");
        }

        window.console.log("Re-set the counter for the number of running requests and responses");

        //Re-set the client
        re_set_client();

        window.console.log("The connection to: '" + module.url + "'has failed!");
    }
    
    //Create the client module
    module = create_ws_client_fn(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 on_open, on_message, on_close, escape_html_fn,
                                 request_progress_bar, response_progress_bar);
    
    //Declare the supported languages request
    SUPPORTED_LANG_REQ = {"prot_ver" : module.PROTOCOL_VERSION, "msg_type" : module.MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_REQ};
    //Declare the supported languages request
    TRAN_JOB_REQ_BASE = {"prot_ver" : module.PROTOCOL_VERSION, "msg_type" : module.MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_REQ};

    /**
     * Allows to send a new translation request to the server
     * @param {String} source_lang the source language string
     * @param {String} target_lang the target languaghe string
     * @param {Bool} is_trans_info true if the translation info is to be requested, otherwise false
     * @param {array of strings} source_sent an array of prepared sentences to be sent in the translation job
     */
    function send_translation_request(source_lang, target_lang, is_trans_info, source_sent) {
        //Increment the number of active translations
        sent_trans_req += 1;
        window.console.log("A new request, #jobs: " + sent_trans_req);

        //Get the new translation job request id
        prev_job_req_id += 1;

        //Initialize and construct the json translation job request
        var trans_job_req = TRAN_JOB_REQ_BASE;

        //Set the translation job id
        trans_job_req.job_id = prev_job_req_id;

        //Set the source language
        trans_job_req.source_lang = source_lang;

        //Set the target language
        trans_job_req.target_lang = target_lang;

        //Get the translation info flag from thecheckox!
        trans_job_req.is_trans_info = is_trans_info;

        //Set the source text split line by line
        trans_job_req.source_sent = source_sent;

        window.console.log("Sending a sentence array with " + source_sent.length + " sentences");

        //Send a new translation request
        module.send_request_fn(trans_job_req);
    }
    
    /**
     * Allows to translate the given source text from the given source language
     * @param source_text {String} the source text to translate
     * @param job_token {String} the pre-processed source text to translate
     * @param source_lang {String} the source language name
     */
    function do_translate(source_text, token, source_lang) {
        //Check that the translation server is online, otherwise it does not make sence to proceed
        if (module.is_connected_fn()) {
            window.console.log("The translation server is connected!");
            
            //Declare the local variables
            var sent_array, begin_idx, end_idx, target_lang, is_trans_info;
            
            //Store the job token
            job_token = token;
            
            //Get the target language
            target_lang = lang_mdl.get_sel_target_lang_fn();

            //Get the translation info flag from thecheckox!
            is_trans_info = trans_info_cb.is(':checked');

            sent_array = source_text.split('\n');
            window.console.log("Send the translation requests for " + sent_array.length + " sentences");

            //Set the translating flag
            is_working = true;

            //Re-initialize the progress bars
            module.init_req_resp_pb_fn();

            begin_idx = 0;
            while (begin_idx < sent_array.length) {
                //Compute the end index
                end_idx = window.Math.min(begin_idx + MAX_NUM_SENTENCES_PER_JOB, sent_array.length);
                window.console.log("Sending sentences [" + begin_idx + "," + end_idx + ")");
                send_translation_request(source_lang, target_lang, is_trans_info, sent_array.slice(begin_idx, end_idx));
                begin_idx = end_idx;
                module.set_request_pb_fn(end_idx, sent_array.length);
            }

            //Make the progress note visible
            module.set_response_pb_fn(0, 1);

            logger_mdl.success("Sent out " + sent_trans_req + " translation requests");
        } else {
            process_stop_fn(true, "The translation server is not connected!");
        }
    }
    
    //Set the translation function
    module.process_fn = do_translate;
    
    return module;
}
