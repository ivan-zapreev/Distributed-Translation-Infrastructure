/**
 * Allows to create a new translation server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the translation server client module
 */
function create_translation_client(logger_mdl, lang_mdl, url_input, url,
                                    server_cs_img, server_cs_bage,
                                    needs_new_trans_fn, disable_interface_fn,
                                    enable_interface_fn, create_ws_client_fn) {
    "use strict";
    
    var SUPPORTED_LANG_REQ, TRAN_JOB_REQ_BASE, is_translating, sent_trans_req, received_trans_resp, module;
    
    //Do default initializations
    is_translating = false;
    sent_trans_req = 0;
    received_trans_resp = 0;
    
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
        window.console.log("Message is received, parsing to JSON");
        
        //Check of the message type
        if (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_RESP) {
            //ToDo: Set the translation data
            set_translation(resp_obj);
        } else {
            if (resp_obj.msg_type === module.MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_RESP) {
                //ToDo: Set the supported languages
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
        if (is_translating) {
            logger_mdl.danger("Falied to perform translation the server dropped off!", true);
            //Set the translating flag back to false
            is_translating = false;
            //ToDo: Re-initialize the progress bars
            //initialize_progress_bars();
        } else {
            logger_mdl.warning("The connection to '" + module.url + "' is closed");
        }

        window.console.log("Re-set the counter for the number of running requests and responses");
        sent_trans_req = 0;
        received_trans_resp = 0;

        //ToDo: Update the translation statis
        //update_trans_status();

        window.console.log("The connection to: '" + module.url + "'has failed!");
    }
    
    //Create the client module
    module = create_ws_client_fn(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 on_open, on_message, on_close);
    
    //Declare the supported languages request
    SUPPORTED_LANG_REQ = {"prot_ver" : module.PROTOCOL_VERSION, "msg_type" : module.MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_REQ};
    //Declare the supported languages request
    TRAN_JOB_REQ_BASE = {"prot_ver" : module.PROTOCOL_VERSION, "msg_type" : module.MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_REQ};
    
    return module;
}
