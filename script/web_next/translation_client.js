//Store the undefined job id value
var UNDEFINED_JOB_ID = 0;
//Stores the minimum translation job id
var MINIMUM_TRANS_JOB_ID = 1;

/**
 * Allows to create a new translation server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the translation server client module
 */
function create_trans_client(logger_mdl, lang_mdl, post_serv_mdl, url_input,
                             url, server_cs_img, server_cs_bage,
                             needs_new_trans_fn, disable_interface_fn,
                             enable_interface_fn, create_ws_client_fn,
                             escape_html_fn, request_progress_bar,
                             response_progress_bar, process_stop_fn) {
    "use strict";
    
    var SUPPORTED_LANG_REQ, TRAN_JOB_REQ_BASE, is_working, sent_requests, received_responces, job_responces, module;
    
    //Do default initializations
    is_working = false;
    sent_requests = 0;
    received_responces = 0;
    job_responces = [];
    
    /**
     * Allows to account for a new translation job response
     */
    function count_trans_job_response() {
        //Decrement the number of active translations
        received_responces += 1;
        window.console.log("A new response, remaining #jobs: " + (sent_requests - received_responces));

        //Set the current value for the responses, the maximum is equal to the number of requests
        module.set_response_pb_fn(received_responces, sent_requests);

        //If all the responses are received
        if (sent_requests === received_responces) {
            //We received as many responses as there were requests
            sent_requests = 0;
            received_responces = 0;
            is_working = false;

            //ToDo: Process the job responses, if the post-processor is
            //      online then send data into it, otherwise visualize.
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
        sent_requests = 0;
        received_responces = 0;
        job_responces = [];

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
    
    //Set the translation function
    module.process_fn = function (source_text, job_token, source_lang) {
        //Check that the translation server is online, otherwise it does not make sence to proceed
        if (module.is_connected_fn()) {
            window.console.log("The translation server is connected!");
            
            //ToDo: Implement
        } else {
            process_stop_fn(true, "The translation server is not connected!");
        }
    };
    
    return module;
}
