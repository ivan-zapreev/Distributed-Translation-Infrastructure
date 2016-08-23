/**
 * Allows to create a new post-processor server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the post-processor server client module
 */
function create_post_proc_client(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 create_ws_client_fn, escape_html_fn,
                                 request_progress_bar, response_progress_bar,
                                 process_stop_fn) {
    "use strict";
    
    var module;
    
    module = create_ws_client_fn(logger_mdl, url_input, url, server_cs_img,
                                 server_cs_bage, needs_new_trans_fn,
                                 disable_interface_fn, enable_interface_fn,
                                 null, null, null, escape_html_fn,
                                 request_progress_bar, response_progress_bar);
        
    return module;
}
