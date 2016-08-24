/**
 * Define an empty client module, global variable
 */
var client_module = {
    pre_serv_mdl : null,     //The pre-processor server module object
    trans_serv_mdl : null,   //The translation server module object
    post_serv_mdl : null,    //The post-processor server module object
    logger_mdl : null,       //The logger server module object
    file_ud_mdl : null,      //The file upload/download module
    language_mdl : null,     //The source/target languages selection module
    source_md5 : null,       //Stores the previously translated source text md5 sum
    dom : {}                 //Stores the references to the main dom elements
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
function create_client(config, md5_fn, download_fn, init_file_ud_fn,
                       create_logger_fn, create_lang_fn, create_ws_client_fn,
                       create_proc_client_fn, create_pre_proc_client_fn,
                       create_trans_client_fn, create_post_proc_client_fn) {
    "use strict";
    
    //Get the main dom element references and store them
    client_module.dom.pre_url_input = window.$("#pre_server_url");
    client_module.dom.trans_url_input = window.$("#trans_server_url");
    client_module.dom.post_url_input = window.$("#post_server_url");
    client_module.dom.trans_btn = window.$("#trans_btn");
    client_module.dom.trans_info_cb = window.$("#trans_info_cb");
    client_module.dom.from_text_area = window.$("#from_text");
    client_module.dom.from_lang_sel = window.$("#from_lang_sel");
    client_module.dom.to_text_span = window.$("#to_text");
    client_module.dom.to_lang_sel = window.$("#to_lang_sel");
    client_module.dom.input_file_select = window.$("#input_file_select");
    client_module.dom.progress_image = window.$("#progress");

    //Instantiate the logger module
    (function () {
        var log_panel, lp_dang, lp_warn, lp_info, lp_succ;
        
        log_panel = window.$("#log_panel");
        lp_dang = window.$("#lp_dang");
        lp_warn = window.$("#lp_warn");
        lp_info = window.$("#lp_info");
        lp_succ = window.$("#lp_succ");
        
        client_module.logger_mdl = create_logger_fn(log_panel, lp_dang, lp_warn, lp_info,
                                                    lp_succ, get_date_string, escape_html);
    }());
    //Set the log clearing button handler
    window.$("#log_clear_btn").click(client_module.logger_mdl.clear_log);
    
    //Initialize the file upload/download capabilities
    (function () {
        var input_file_sc, download_text_link, download_log_link;
        
        input_file_sc = window.$("#input_file_sc");
        download_text_link = window.$("#download_text_link");
        download_log_link = window.$("#download_log_link");
    
        init_file_ud_fn(client_module.logger_mdl, client_module.dom.from_text_area,
                        input_file_sc, client_module.dom.input_file_select,
                        download_text_link, download_log_link, download_fn,
                        get_date_string);
    }());

    /**
     * This function must be called in case one needs a new translation when the
     * translation is requests, even if the given text has already been translated.
     * I.e. this function is to be called when some client options change.
     */
    function needs_new_trans() {
        client_module.source_md5 = null;
    }
    
    //Set the function for change handlers
    client_module.dom.trans_info_cb.change(needs_new_trans);
    client_module.dom.to_lang_sel.change(needs_new_trans);
    
    //Initialize the languages module
    (function () {
        client_module.language_mdl = create_lang_fn(client_module.logger_mdl,
                                                    client_module.dom.from_lang_sel,
                                                    client_module.dom.to_lang_sel,
                                                    needs_new_trans);
    }());
    
    /**
     * This function allows to disable the interface
     */
    function disable_interface() {
        window.console.log("Disable the controls");
        
        client_module.dom.pre_url_input.prop("disabled", true);
        client_module.dom.trans_url_input.prop("disabled", true);
        client_module.dom.post_url_input.prop("disabled", true);
        client_module.dom.trans_btn.prop("disabled", true);
        client_module.dom.trans_info_cb.prop("disabled", true);
        client_module.dom.from_text_area.prop("disabled", true);
        client_module.dom.from_lang_sel.prop("disabled", true);
        client_module.dom.to_lang_sel.prop("disabled", true);
        client_module.dom.input_file_select.prop("disabled", true);
    }

    /**
     * This function allows to enable or partially enable the interface
     */
    function enable_interface() {
        window.console.log("Enable the controls");

        client_module.dom.pre_url_input.prop("disabled", false);
        client_module.dom.trans_url_input.prop("disabled", false);
        client_module.dom.post_url_input.prop("disabled", false);
        //if (client_module.trans_serv_mdl.is_connected_fn()) {
        client_module.dom.trans_btn.prop("disabled", false);
        client_module.dom.trans_info_cb.prop("disabled", false);
        client_module.dom.from_text_area.prop("disabled", false);
        client_module.dom.from_lang_sel.prop("disabled", false);
        client_module.dom.to_lang_sel.prop("disabled", false);
        client_module.dom.input_file_select.prop("disabled", false);
        //}
    }

    /**
     * This function allows to disable the interface start the translation progress rotation.
     */
    function process_start() {
        //Disable the interface
        disable_interface();
        //Start the progress image
        client_module.dom.progress_image.attr('src', './img/globe32.gif');
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
        client_module.dom.progress_image.attr('src', './img/globe32.png');
        //Check if there was an error
        if (is_error) {
            client_module.logger_mdl.danger(message, true);
        }
    }
        
    //Instantiate the post-processor module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#post_server_cs");
        server_cs_bage = window.$("#post_sb");
        req_bp = window.$("#post_req_pb");
        resp_pb = window.$("#post_resp_pb");
        
        //Create the processor client module
        client_module.post_serv_mdl = create_proc_client_fn(client_module.logger_mdl,
                                                            client_module.language_mdl,
                                                            client_module.dom.post_url_input,
                                                            config.post_proc_url,
                                                            server_cs_img, server_cs_bage,
                                                            needs_new_trans, disable_interface,
                                                            enable_interface, create_ws_client_fn,
                                                            escape_html, req_bp, resp_pb, process_stop);
        //Upgrade to the post processor module
        create_post_proc_client_fn(client_module.post_serv_mdl);
    }());
    
    //Instantiate the translator module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#trans_server_cs");
        server_cs_bage = window.$("#trans_sb");
        req_bp = window.$("#trans_req_pb");
        resp_pb = window.$("#trans_resp_pb");
        
        client_module.trans_serv_mdl = create_trans_client_fn(client_module.logger_mdl,
                                                              client_module.language_mdl,
                                                              client_module.post_serv_mdl,
                                                              client_module.dom.trans_url_input,
                                                              config.translate_url,
                                                              server_cs_img, server_cs_bage, client_module.dom.trans_info_cb,
                                                              needs_new_trans, disable_interface,
                                                              enable_interface, create_ws_client_fn,
                                                              escape_html, req_bp, resp_pb, process_stop);
    }());
    
    //Instantiate the pre-processor module
    (function () {
        var url_input, server_cs_img, server_cs_bage, req_bp, resp_pb;
        
        server_cs_img = window.$("#pre_server_cs");
        server_cs_bage = window.$("#pre_sb");
        req_bp = window.$("#pre_req_pb");
        resp_pb = window.$("#pre_resp_pb");
        
        //Create the processor client module
        client_module.pre_serv_mdl = create_proc_client_fn(client_module.logger_mdl,
                                                           client_module.language_mdl,
                                                           client_module.dom.pre_url_input,
                                                           config.pre_proc_url,
                                                           server_cs_img, server_cs_bage,
                                                           needs_new_trans, disable_interface,
                                                           enable_interface, create_ws_client_fn,
                                                           escape_html, req_bp, resp_pb, process_stop);
        //Upgrade to the post processor module
        create_pre_proc_client_fn(client_module.pre_serv_mdl, client_module.trans_serv_mdl);
    }());
    
    //Set the tranlate button handler
    client_module.dom.trans_btn.click(function () {
        var source_text, source_md5, sent_array, begin_idx, end_idx, source_lang, target_lang, is_trans_info;

        //Get and prepare the new source text
        source_text = client_module.dom.from_text_area.val().trim();
        window.console.log("The text to translate is: " + source_text.substr(0, 128) + "...");

        //First check that the text is not empty
        if (source_text !== "") {
            //Compute the new source md5 value
            source_md5 = md5_fn(source_text);
            window.console.log("The new source md5: " + source_md5);

            //Now check that the md5 sum has changed
            if (source_md5 !== client_module.source_md5) {
                window.console.log("The new text is now empty and is different from the previous");

                //Store the new previous translation request md5
                client_module.source_md5 = source_md5;

                //Check if the target language is selected
                if (client_module.language_mdl.is_target_lang_sel_fn()) {
                    //Clear the current translation text
                    client_module.dom.to_text_span.html("");
                    
                    //Start the process
                    process_start();
                    
                    //Start the process by calling the pre-processor module
                    client_module.pre_serv_mdl.process_fn(source_md5, source_text);
                } else {
                    client_module.logger_mdl.danger("Please selecte the target language!", true);
                }
            } else {
                client_module.logger_mdl.warning("This translation job has already been done!", true);
            }
        } else {
            client_module.logger_mdl.warning("There is no text to translate!", true);
        }
    });
    
    //Connect to the clients
    client_module.pre_serv_mdl.connect_fn();
    client_module.trans_serv_mdl.connect_fn();
    client_module.post_serv_mdl.connect_fn();
}
