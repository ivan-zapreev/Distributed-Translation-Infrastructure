/**
 * Define an empty client module, global variable
 */
var translator = {
    logger_mdl : null,        //The logger server module object
    language_mdl : null,      //The source/target languages selection module
    file_ud_mdl : null,       //The file upload/download module
    common_mdl : null,        //The common module shared by all the server modules
    pre_serv_mdl : null,      //The pre-processor server module object
    trans_serv_mdl : null,    //The translation server module object
    post_serv_mdl : null,     //The post-processor server module object
    source_md5 : null,        //Stores the previously translated source text md5 sum
    dom : {}                  //Stores the references to the main dom elements
};

/**
 * This function allows to get data/time in a short format
 * @param {String} delim the delimiter between the date and time, default is " "
 * @return the date string, used for logging and other things
 */
function get_date_string(delim) {
    "use strict";
    
    delim = delim || " ";

    var date = new Date();
    return date.toLocaleDateString() + delim + date.toLocaleTimeString();
}

/**
 * Allows to escape the HTML characters
 * @param {String} the unsafe string
 * @return the escaped string
 */
function escape_html(unsafe) {
    "use strict";

    return unsafe
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#039;");
}

/**
 * This function is called in the beginning just after the DOM tree
 * is parsed and ensures starting up and runnning the web interface.
 * @param config {Object} the configuration object
 * @param md5_fn {Function} the function to compute MD5
 * @param download_fn {Function} the function to download file from JavaScript
 * @param init_file_ud_fn {Function} the function to initialize the file upload/download capabilities
 * @param create_logger_fn {Function} the function to create a logger module instance
 * @param create_ws_client_fn {Function} the function to create a web socket client module instance
 */
function initialize_translator(config, md5_fn, download_fn, init_file_ud_fn,
                               create_logger_fn, create_lang_fn,
                               create_client_common_fn, create_ws_client_fn,
                               create_proc_client_fn, create_pre_proc_client_fn,
                               create_trans_client_fn, create_post_proc_client_fn) {
    "use strict";
    
    //Get the main dom element references and store them
    translator.dom.pre_url_input = window.$("#pre_server_url");
    translator.dom.trans_url_input = window.$("#trans_server_url");
    translator.dom.post_url_input = window.$("#post_server_url");
    translator.dom.trans_btn = window.$("#trans_btn");
    translator.dom.trans_info_cb = window.$("#trans_info_cb");
    translator.dom.from_text_area = window.$("#from_text");
    translator.dom.from_lang_sel = window.$("#from_lang_sel");
    translator.dom.to_text_span = window.$("#to_text");
    translator.dom.to_lang_sel = window.$("#to_lang_sel");
    translator.dom.input_file_select = window.$("#input_file_select");
    translator.dom.progress_image = window.$("#progress");
    //Enable tool-tipls
    window.$('[data-toggle="tooltip"]').tooltip();

    //Declare the translation status variable that
    //will store the comulative translation status
    var trans_status = 0;
    
    //Instantiate the logger module
    (function () {
        var log_panel, lp_dang, lp_warn, lp_info, lp_succ;
        
        log_panel = window.$("#log_panel");
        lp_dang = window.$("#lp_dang");
        lp_warn = window.$("#lp_warn");
        lp_info = window.$("#lp_info");
        lp_succ = window.$("#lp_succ");
        
        translator.logger_mdl = create_logger_fn(log_panel, lp_dang, lp_warn, lp_info,
                                                    lp_succ, get_date_string, escape_html);
    }());
    //Set the log clearing button handler
    window.$("#log_clear_btn").click(translator.logger_mdl.clear_log);
    
    //Initialize the file upload/download capabilities
    (function () {
        var input_file_sc, download_text_link, download_log_link;
        
        input_file_sc = window.$("#input_file_sc");
        download_text_link = window.$("#download_text_link");
        download_log_link = window.$("#download_log_link");
    
        init_file_ud_fn(translator.logger_mdl, translator.dom.from_text_area,
                        input_file_sc, translator.dom.input_file_select,
                        download_text_link, download_log_link, download_fn,
                        get_date_string);
    }());

    /**
     * This function must be called in case one needs a new translation when the
     * translation is requests, even if the given text has already been translated.
     * I.e. this function is to be called when some client options change.
     */
    function needs_new_trans() {
        translator.source_md5 = null;
    }
    
    //Set the function for change handlers
    translator.dom.trans_info_cb.change(needs_new_trans);
    translator.dom.to_lang_sel.change(needs_new_trans);
    
    //Initialize the languages module
    (function () {
        translator.language_mdl = create_lang_fn(translator.logger_mdl,
                                                    translator.dom.from_lang_sel,
                                                    translator.dom.to_lang_sel,
                                                    needs_new_trans);
    }());
    
    /**
     * This function allows to disable the interface
     */
    function disable_interface() {
        window.console.log("Disable the controls");
        
        translator.dom.pre_url_input.prop("disabled", true);
        translator.dom.trans_url_input.prop("disabled", true);
        translator.dom.post_url_input.prop("disabled", true);
        translator.dom.trans_btn.prop("disabled", true);
        translator.dom.trans_info_cb.prop("disabled", true);
        translator.dom.from_text_area.prop("disabled", true);
        translator.dom.from_lang_sel.prop("disabled", true);
        translator.dom.to_lang_sel.prop("disabled", true);
        translator.dom.input_file_select.prop("disabled", true);
    }

    /**
     * This function allows to enable or partially enable the interface
     */
    function enable_interface() {
        window.console.log("Enable the controls");

        translator.dom.pre_url_input.prop("disabled", false);
        translator.dom.trans_url_input.prop("disabled", false);
        translator.dom.post_url_input.prop("disabled", false);
        //if (translator.trans_serv_mdl.is_connected_fn()) {
        translator.dom.trans_btn.prop("disabled", false);
        translator.dom.trans_info_cb.prop("disabled", false);
        translator.dom.from_text_area.prop("disabled", false);
        translator.dom.from_lang_sel.prop("disabled", false);
        translator.dom.to_lang_sel.prop("disabled", false);
        translator.dom.input_file_select.prop("disabled", false);
        //}
    }

    /**
     * This function allows to disable the interface start the translation progress rotation.
     */
    function process_start() {
        //Disable the interface
        disable_interface();
        //Start the progress image
        translator.dom.progress_image.attr('src', './img/globe32.gif');
    }

    /**
     * This function allows to enable the interface and report on an error, if any, also stops the progress rotation
     * @param is_error {Boolean} true if we stop in case of an error, false if we stop because we are finished
     * @param message {String} the message string to be used in case we stop in an error case
     */
    function process_stop(is_error, message) {
        //Enable the interface
        enable_interface();
        //Stop the progress image
        translator.dom.progress_image.attr('src', './img/globe32.png');
        //Check if there was an error
        if (is_error) {
            //Log the message and notify the user
            translator.logger_mdl.danger(message, true);
            //The new translation is needed, as there as an error
            needs_new_trans();
        }
    }
    
    //Instantiate the client common module
    (function () {
        //Create the processor client module
        translator.common_mdl = create_client_common_fn(translator.logger_mdl,
                                                        translator.language_mdl,
                                                        translator.dom.to_text_span,
                                                        create_ws_client_fn, needs_new_trans,
                                                        enable_interface, disable_interface,
                                                        escape_html, process_stop);
    }());
     
    //Instantiate the post-processor module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#post_server_cs");
        server_cs_bage = window.$("#post_sb");
        req_bp = window.$("#post_req_pb");
        resp_pb = window.$("#post_resp_pb");
        
        //Create the processor client module
        translator.post_serv_mdl = create_proc_client_fn(translator.common_mdl,
                                                         translator.dom.post_url_input,
                                                         config.post_proc_url, server_cs_img,
                                                         server_cs_bage, req_bp, resp_pb, "Post");
        //Upgrade to the post processor module
        create_post_proc_client_fn(translator.post_serv_mdl);
    }());
    
    //Instantiate the translator module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#trans_server_cs");
        server_cs_bage = window.$("#trans_sb");
        req_bp = window.$("#trans_req_pb");
        resp_pb = window.$("#trans_resp_pb");
        
        translator.trans_serv_mdl = create_trans_client_fn(translator.common_mdl,
                                                           translator.post_serv_mdl,
                                                           translator.dom.trans_url_input,
                                                           config.translate_url,
                                                           server_cs_img, server_cs_bage,
                                                           translator.dom.trans_info_cb,
                                                           req_bp, resp_pb);
    }());
    
    //Instantiate the pre-processor module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#pre_server_cs");
        server_cs_bage = window.$("#pre_sb");
        req_bp = window.$("#pre_req_pb");
        resp_pb = window.$("#pre_resp_pb");
        
        //Create the processor client module
        translator.pre_serv_mdl = create_proc_client_fn(translator.common_mdl,
                                                        translator.dom.pre_url_input,
                                                        config.pre_proc_url, server_cs_img,
                                                        server_cs_bage, req_bp, resp_pb, "Pre");
        //Upgrade to the post processor module
        create_pre_proc_client_fn(translator.pre_serv_mdl, translator.trans_serv_mdl);
    }());
    
    //Set the tranlate button handler
    translator.dom.trans_btn.click(function () {
        var source_text, source_md5, sent_array, begin_idx, end_idx, source_lang, target_lang, is_trans_info;

        //Get and prepare the new source text
        source_text = translator.dom.from_text_area.val().trim();
        window.console.log("The text to translate is: " + source_text.substr(0, 128) + "...");

        //First check that the text is not empty
        if (source_text !== "") {
            //Compute the new source md5 value
            source_md5 = md5_fn(source_text);
            window.console.log("The new source md5: " + source_md5);

            //Now check that the md5 sum has changed
            if (source_md5 !== translator.source_md5) {
                window.console.log("The new text is now empty and is different from the previous");

                //Store the new previous translation request md5
                translator.source_md5 = source_md5;

                //Check if the target language is selected
                if (translator.language_mdl.is_target_lang_sel_fn()) {
                    //Clear the current translation text
                    translator.dom.to_text_span.html("");
                    
                    //Re-set the translation status
                    translator.common_mdl.remove_cs_visual_fn();
                    
                    //Re-set the progress bars of the modules
                    translator.pre_serv_mdl.init_req_resp_pb_fn();
                    translator.trans_serv_mdl.init_req_resp_pb_fn();
                    translator.post_serv_mdl.init_req_resp_pb_fn();
                    
                    //Start the process
                    process_start();
                    
                    //Start the process by calling the pre-processor module
                    translator.pre_serv_mdl.process_fn(source_text, source_md5);
                } else {
                    translator.logger_mdl.danger("Please selecte the target language!", true);
                }
            } else {
                translator.logger_mdl.warning("This translation job has already been done!", true);
            }
        } else {
            translator.logger_mdl.warning("There is no text to translate!", true);
        }
    });
    
    //Connect to the clients
    translator.pre_serv_mdl.connect_fn();
    translator.trans_serv_mdl.connect_fn();
    translator.post_serv_mdl.connect_fn();
}
