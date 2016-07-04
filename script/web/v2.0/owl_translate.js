var client_data = {
    "server_url" : "ws://localhost:9002", //The web server url
    "ws" : null,                          //The web socket to the server
    "sent_trans_req" : 0,                 //The number of sent translation requests
    "received_trans_resp" : 0,            //The number of received translation responses
    "job_responces" : [],                 //Stores the translation job responses
    "translation_html" : "",              //Stores the translation HTML data
    "prev_job_req_id" : -1,               //Stores the previously send translation job request id
    "prev_source_md5" : "",               //Stores the previously sent translation job source text
    "calcMD5" : null,                     //The md5 function, to be initialized
    "server_url_inpt" : null              //Stores the server url input
};

//The communication protocol version value
var PROTOCOL_VERSION = 1;
//The maximum number of sentences per translation job request.
var MAX_NUM_SENTENCES_PER_JOB = 128;
//This enumeration stores the available message type values
var MSG_TYPE_ENUM = {
    //The message type is undefined
    MESSAGE_UNDEFINED : 0,
    //The supported languages request message
    MESSAGE_SUPP_LANG_REQ : 1,
    //The supported languages response message
    MESSAGE_SUPP_LANG_RESP : 2,
    //The translation job request message
    MESSAGE_TRANS_JOB_REQ : 3,
    //The translation job response message
    MESSAGE_TRANS_JOB_RESP : 4
};

//This enumeration stores the status code value
var STATUS_CODE_ENUM = {
    RESULT_UNDEFINED : 0,
    RESULT_UNKNOWN : 1,
    RESULT_OK : 2,
    RESULT_ERROR : 3,
    RESULT_CANCELED : 4,
    RESULT_PARTIAL : 5
};

//Declare the supported languages request
var SUPPORTED_LANG_REQ = {"prot_ver" : PROTOCOL_VERSION, "msg_type" : MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_REQ};
//Declare the supported languages request
var TRAN_JOB_REQ_BASE = {"prot_ver" : PROTOCOL_VERSION, "msg_type" : MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_REQ};

//Declare the "Please select" string for the srouce/target language select boxes
var PLEASE_SELECT_STRING = "Please select";

/**
 * This function must be called in case one needs a new translation when the
 * translation is requests, even if the given text has already been translated.
 * I.e. this function is to be called when some client options change.
 */
function require_new_translation() {
    "use strict";

    client_data.prev_source_md5 = null;
}

/**
 * This function allows to update the current connection status
 * in the GUI. We use the same values as the Websocket connection.
 * @param {Number} ws_status the Webscoket status value to be stored
 */
function update_conn_status(ws_status) {
    "use strict";

    switch (ws_status) {
    case window.WebSocket.CONNECTING:
        client_data.conn_status_span.innerHTML = "Connecting ...";
        break;
    case window.WebSocket.OPEN:
        client_data.conn_status_span.innerHTML = "Connected";
        break;
    case window.WebSocket.CLOSING:
        client_data.conn_status_span.innerHTML = "Disconnecting ...";
        break;
    case window.WebSocket.CLOSED:
        client_data.conn_status_span.innerHTML = "Disconnected";
        break;
    default:
        client_data.conn_status_span.innerHTML = "Puzzled :)";
        break;
    }
}

/**
 * Allows to translate the status code number into a value
 * @param {Number} status_code the status code received from the server
 * @return {String} the string representation of the status code
 */
function get_status_code_string(status_code) {
    "use strict";
    
    switch (status_code) {
    case STATUS_CODE_ENUM.RESULT_OK:
        return "FULLY TRANSLATED";
    case STATUS_CODE_ENUM.RESULT_ERROR:
        return "FAILED TO TRANSLATE";
    case STATUS_CODE_ENUM.RESULT_CANCELED:
        return "CANCELED TRANSLATION";
    case STATUS_CODE_ENUM.RESULT_PARTIAL:
        return "PARTIALY TRANSLATED";
    default:
        return "oooOPS, unknown";
    }
}

/**
 * Allows to visualize the status code and message if needed
 * @param {Number} stat_code the job response status code
 * @param {String} stat_msg the status message string the status message
 */
function visualize_status_code(stat_code, stat_msg) {
    "use strict";
    
    switch (stat_code) {
    case STATUS_CODE_ENUM.RESULT_OK:
        client_data.to_text_span.style.boxShadow = "0 0 10px green";
        break;
    case STATUS_CODE_ENUM.RESULT_ERROR:
        client_data.to_text_span.style.boxShadow = "0 0 10px red";
        window.alert("Server status: " + stat_msg);
        break;
    case STATUS_CODE_ENUM.RESULT_CANCELED:
        window.alert("Server status: " + stat_msg);
        client_data.to_text_span.style.boxShadow = "0 0 10px orange";
        break;
    case STATUS_CODE_ENUM.RESULT_PARTIAL:
        window.alert("Server status: " + stat_msg);
        client_data.to_text_span.style.boxShadow = "0 0 10px yellow";
        break;
    default:
        client_data.to_text_span.style.boxShadow = "none";
        break;
    }
}

/**
 * Allows to re-set the status code visualization
 */
function remove_status_code_visual() {
    "use strict";
    
    visualize_status_code(STATUS_CODE_ENUM.RESULT_UNDEFINED, "");
}

/**
 * This function allows to disable the interface
 */
function disable_interface() {
    "use strict";

    window.console.log("Disable the controls");
    
    client_data.server_url_inpt.disabled = true;
    client_data.trans_btn.disabled = true;
    client_data.trans_info_cb.disabled = true;
    client_data.from_text_area.disabled = true;
    client_data.from_lang_sel.disabled = true;
    client_data.to_lang_sel.disabled = true;
}

/**
 * This function allows to enable or partially enable the interface
 * @param {Boolean} is_connected true if we enable the controls when
 *                  we are connected to a server, otherwise false.
 */
function enable_interface(is_connected) {
    "use strict";
    
    window.console.log("Enable the controls");
    
    client_data.server_url_inpt.disabled = false;
    if (is_connected) {
        client_data.trans_btn.disabled = false;
        client_data.trans_info_cb.disabled = false;
        client_data.from_text_area.disabled = false;
        client_data.from_lang_sel.disabled = false;
        client_data.to_lang_sel.disabled = false;
    }
}

/**
 *
 *
 *
 *
 */
function set_progress_bar(pb, msg, curr_num, max_num) {
    "use strict";
    
    window.console.log("The current value: " + curr_num + " max value: " + max_num);
    
    var percent = window.Math.round((curr_num / max_num) * 100);
    pb.html(msg + ": " + percent + "%");
    window.console.log(pb.html());
    pb.width(percent + "%");
    pb.attr("aria-valuenow", percent);
    
    if (percent === 0) {
        pb.addClass("active");
    } else {
        if (percent === 100) {
            pb.removeClass("active");
        }
    }
}

/**
 *
 *
 *
 */
function set_response_progress_bar(curr_received, max_received) {
    "use strict";
    
    set_progress_bar(client_data.response_progress_bar, "Responses: ", curr_received, max_received);
}

/**
 *
 *
 *
 */
function set_request_progress_bar(num_sent, max_sent) {
    "use strict";
    
    set_progress_bar(client_data.request_progress_bar, "Requests: ", num_sent, max_sent);
}

/**
 * This function allows to update the translation progress bar status.
 * It acts depending on the current number of active translation requests.
 * This function is called after a new translation is requested or the
 * connection is dropped or a translation response is received.
 */
function update_trans_status() {
    "use strict";

    window.console.log("Update the translation status");
    
    if (client_data.sent_trans_req === client_data.received_trans_resp) {
        client_data.progress_image.src = "globe32.png";
    } else {
        client_data.progress_image.src = "globe32.gif";
    }
}

/**
 * Allows to fill int he single response data into the target data span
 * @param {Object} trans_response the translation reponse object
 */
function fill_in_single_response_data(trans_response, response_idx, trans_responses) {
    "use strict";

    //Declare the variables to be used
    var j, target, status;
    
    //Set the border color based on the overall status
    visualize_status_code(trans_response.stat_code, trans_response.stat_msg);

    //Only visualize the results if the target data is present
    if (trans_response.stat_code !== STATUS_CODE_ENUM.RESULT_ERROR &&
            trans_response.stat_code !== STATUS_CODE_ENUM.RESULT_CANCELED) { //trans_response.hasOwnProperty('target_data')
        //Assemble the data
        for (j = 0; j < trans_response.target_data.length; j += 1) {
            //Get the target
            target = trans_response.target_data[j];
            //Create the status string, to hover
            status = get_status_code_string(target.stat_code);
            //Add the message if it is not empty
            if (target.stat_code !== STATUS_CODE_ENUM.RESULT_OK) {
                status += ": " + target.stat_msg;
            } else {
                //Check if the stack loads are present, if yes, add them
                if (client_data.trans_info_cb.checked) { //target.hasOwnProperty('stack_load')
                    status += ", multi-stack loads(%): [" + target.stack_load.join(" ") + "]";
                }
            }
            //Add the translation element to the panel
            client_data.translation_html += "<span class='target_sent_tag' title='' data-original-title='" +
                status + "' data-toggle='tooltip' data-placement='top'>" + target.trans_text + "</span>";
        }
    }
    
    //Check if this is the last response
    if (response_idx === (trans_responses.length - 1)) {
        //Set the translation html into the DOM tree just once, otherwise it is too slow!
        client_data.to_text_span.innerHTML = client_data.translation_html;
        window.$('[data-toggle="tooltip"]').tooltip();
        client_data.translation_html = "";
        
        //Update the translation status
        update_trans_status();

        //Enable the interface controls
        enable_interface(true);
    }
}

/**
 * Allows to process a large array in an asynchronous way
 * @param {array} array of data to process 
 * @param {function} the call back function to be called per array element
 * @param {time} the time allowed to be busy per batch, optional
 * @param {context} context the context, optional
 */
function process_array_async(array, fn, maxTimePerChunk, context) {
    "use strict";
    
    context = context || window;
    maxTimePerChunk = maxTimePerChunk || 200;
    var index = 0;

    function now() {
        var time = new Date().getTime();
        window.console.log("Next iteration time is: " + time);
        return time;
    }

    function doChunk() {
        var startTime = now();
        while (index < array.length && (now() - startTime) <= maxTimePerChunk) {
            // callback called with args (value, index, array)
            fn.call(context, array[index], index, array);
            index += 1;
        }
        if (index < array.length) {
            // set Timeout for async iteration
            setTimeout(doChunk, 1);
        }
    }
    doChunk();
}

/**
 * Allows to account for a new translation job response
 */
function count_trans_job_response() {
    "use strict";
    
    //Decrement the number of active translations
    client_data.received_trans_resp += 1;
    window.console.log("A new response, remaining #jobs: " + (client_data.sent_trans_req - client_data.received_trans_resp));

    //Set the current value for the responses, the maximum is equal to the number of requests
    set_response_progress_bar(client_data.received_trans_resp, client_data.sent_trans_req);
    
    //If all the responses are received
    if (client_data.sent_trans_req === client_data.received_trans_resp) {
        //We received as many responses as there were requests
        client_data.sent_trans_req = 0;
        client_data.received_trans_resp = 0;

        //Get the text element to put the values into
        client_data.translation_html = "";
    
        //Create the response and put it into the text span
        process_array_async(client_data.job_responces, fill_in_single_response_data);
    }
}

/**
 * Allows to account for a new translation job request
 */
function count_trans_job_request() {
    "use strict";
    
    //If this is the first translation jobe request then disable the interface
    if (client_data.sent_trans_req === 0) {
        //Disable the interface
        disable_interface();
    }
    
    //Increment the number of active translations
    client_data.sent_trans_req += 1;
    window.console.log("A new request, #jobs: " + client_data.sent_trans_req);

    //Update the translation status
    update_trans_status();

    //Remove the visualization of the status
    remove_status_code_visual();
}

/**
 * Allows to get the selected element value from the select element with the given id
 * @param {Object} the select element to get the selected value from
 * @param {String} the value of the selected element
 */
function get_selected_lang(select) {
    "use strict";
    
    var selected_lang;
    
    selected_lang = select.options[select.selectedIndex].value;
    window.console.log("The selected '" + select + "' language is: " + selected_lang);
    
    return selected_lang.trim();
}

/**
 * Allows to get the selected language source value
 * @return {string} the value of the selected source language
 */
function get_selected_source_lang() {
    "use strict";

    return get_selected_lang(client_data.from_lang_sel);
}

/**
 * Allows to get the selected language target value
 * @return {string} the value of the selected target language
 */
function get_selected_target_lang() {
    "use strict";

    return get_selected_lang(client_data.to_lang_sel);
}

/**
 * Allows to send a new translation request to the server
 * @param {String} source_lang the source language string
 * @param {String} target_lang the target languaghe string
 * @param {Bool} is_trans_info true if the translation info is to be requested, otherwise false
 * @param {array of strings} source_sent an array of prepared sentences to be sent in the translation job
 */
function send_translation_request(source_lang, target_lang, is_trans_info, source_sent) {
    "use strict";

    //Count the translation job request
    count_trans_job_request();
    
    //Get the new translation job request id
    client_data.prev_job_req_id += 1;
    
    //Initialize and construct the json translation job request
    var trans_job_req = TRAN_JOB_REQ_BASE;
    
    //Set the translation job id
    trans_job_req.job_id = client_data.prev_job_req_id;
    
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
    client_data.ws.send(JSON.stringify(trans_job_req));
}

/**
 * This function is called if a new translation request is to be sent.
 */
function do_translate() {
    "use strict";
    var source_text, new_source_md5, sent_array, begin_idx, end_idx, source_lang, target_lang, is_trans_info;
    
    //Get and prepare the new source text
    source_text = client_data.from_text_area.value.trim();
    window.console.log("The text to translate is: " + source_text.substr(0, 128) + "...");
    
    //First check that the text is not empty
    if (source_text !== "") {
        //Compute the new source md5 value
        new_source_md5 = client_data.calcMD5(source_text);
        window.console.log("The new source md5: " + new_source_md5);
        
        //Now check that the md5 sum has changed
        if (new_source_md5 !== client_data.prev_source_md5) {
            window.console.log("The new text is now empty and is different from the previous");

            //Store the new previous translation request md5
            client_data.prev_source_md5 = new_source_md5;
            
            //Re-set the job id value
            client_data.prev_job_req_id = -1;

            //Re-set the job responses array
            client_data.job_responces = [];

            //Set the source language
            source_lang = get_selected_source_lang();

            //Set the target language
            target_lang = get_selected_target_lang();

            //Get the translation info flag from thecheckox!
            is_trans_info = client_data.trans_info_cb.checked;

            //Clear the current translation text
            client_data.to_text_span.innerHTML = "";

            sent_array = source_text.split('\n');
            window.console.log("Send the translation requests for " + sent_array.length + " sentences");
            set_request_progress_bar(0, sent_array.length);
            set_response_progress_bar(0, sent_array.length);
            begin_idx = 0;
            while (begin_idx < sent_array.length) {
                //Compute the end index
                end_idx = window.Math.min(begin_idx + MAX_NUM_SENTENCES_PER_JOB, sent_array.length);
                window.console.log("Sending sentences [" + begin_idx + "," + end_idx + ")");
                send_translation_request(source_lang, target_lang, is_trans_info, sent_array.slice(begin_idx, end_idx));
                begin_idx = end_idx;
                set_request_progress_bar(end_idx, sent_array.length);
            }
            window.console.log("Finished sending translation request jobs.");
        }
    }
}

/**
 * This function is called in case an error has occured
 * in this script and it needs to be reported. The current
 * implementation uses a simple window alert for that.
 */
function error(message) {
    "use strict";
    
    window.alert("ERROR: " + message);
}

/**
 * This function is called when a connection with the translation server is open.
 */
function on_open() {
    "use strict";
    
    window.console.log("Connection is open");
    update_conn_status(window.WebSocket.OPEN);

    //Sent the request for the supported languages
    var supp_lang_reg_str = JSON.stringify(SUPPORTED_LANG_REQ);
    window.console.log("Sending the supported languages request: " + supp_lang_reg_str);
    client_data.ws.send(supp_lang_reg_str);

    //Enable the interface controls
    enable_interface(true);
}

/**
 * This function is called when a translation response is to be set into the interface
 * Note that the translation response will not be set if it is outdated.
 * @param {String} trans_response the serialized translation job response
 */
function set_translation(trans_response) {
    "use strict";
    
    //Store the translation response in the array
    client_data.job_responces[trans_response.job_id] = trans_response;

    //Cound the translation job response
    count_trans_job_response();
}

/**
 * This function returns the option HTML tag for the select
 * @param {String} value the value for the option
 * @param {String} name the name for the option
 * @return {String} the complete <option> tag
 */
function get_select_option(value, name) {
    "use strict";
    return "<option value=\"" + value + "\">" + name + "</option>";
}

/**
 * This function is called in case once thanges the selection of the source language for the translation.
 * This function then loads the appropriate list of the supported target languages.
 */
function on_source_lang_select() {
    "use strict";
    var i, source_lang, targets;
    
    //A new source language means new translation
    require_new_translation();
        
    window.console.log("The source language is selected");
    source_lang = get_selected_source_lang();

    //Re-set the target select to no options
    client_data.to_lang_sel.innerHTML = "";
    
    if (source_lang !== "") {
        window.console.log("The source language is not empty!");
        targets = client_data.language_mapping[source_lang];

        //Do not add "Please select" in case there is just one target option possible"
        if (targets.length > 1) {
            client_data.to_lang_sel.innerHTML = get_select_option("", PLEASE_SELECT_STRING);
        }

        window.console.log(source_lang + " -> [ " + targets + " ]");
        for (i = 0; i < targets.length; i += 1) {
            client_data.to_lang_sel.innerHTML += get_select_option(targets[i], targets[i]);
        }
    }
}

/**
 * This function allows to set the supported languages from the supported languages response from the server
 * @param {Object} supp_lang_resp the supported languages response message
 */
function set_supported_languages(supp_lang_resp) {
    "use strict";
    var source_lang, num_sources;

    //Store the supported languages
    client_data.language_mapping = supp_lang_resp.langs;

    window.console.log("Clear the to-select as we are now re-loading the source languages");
    client_data.to_lang_sel.innerHTML = "";

    //Do not add "Please select" in case there is just one source option possible"
    num_sources = Object.keys(client_data.language_mapping).length;
    window.console.log("The number of source languages is: " + num_sources);

    if (num_sources > 1) {
        window.console.log("Multiple source languages: Adding 'Please select'");
        client_data.from_lang_sel.innerHTML = get_select_option("", PLEASE_SELECT_STRING);
    } else {
        //Re-set the source select to no options
        client_data.from_lang_sel.innerHTML = "";
    }

    window.console.log("Add the available source languages");
    for (source_lang in client_data.language_mapping) {
        if (client_data.language_mapping.hasOwnProperty(source_lang)) {
            client_data.from_lang_sel.innerHTML += get_select_option(source_lang, source_lang);
        }
    }

    if (num_sources === 1) {
        window.console.log("Single source language, set the targets right away");
        on_source_lang_select();
    }
}

/**
 * This function is called when a message is received from the translation server
 * @param evt the received websocket event 
 */
function on_message(evt) {
    "use strict";
    
    window.console.log("Message is received, parsing to JSON");
    var resp_obj = JSON.parse(evt.data);
    
    //Check of the message type
    if (resp_obj.msg_type === MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_RESP) {
        set_translation(resp_obj);
    } else {
        if (resp_obj.msg_type === MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_RESP) {
            set_supported_languages(resp_obj);
        } else {
            error("An unknown server message: " + evt.data);
        }
    }
}

/**
 * This function is called when a connection with the translation server is dropped.
 */
function on_close() {
    "use strict";
    
    window.console.log("Connection is closed");
    update_conn_status(window.WebSocket.CLOSED);

    window.console.log("Re-set the counter for the number of running requests and responses");
    client_data.sent_trans_req = 0;
    client_data.received_trans_resp = 0;

    //Update the translation statis
    update_trans_status();

    window.console.log("The connection to: '" + client_data.server_url + "'has failed!");

    //Enable the interface controls
    enable_interface(false);
}

/**
 * This function is called if one needs to connect or re-connect to the translation server.
 */
function connect_to_server() {
    "use strict";

    window.console.log("Checking that the web socket connection is available");
    if (window.hasOwnProperty("WebSocket")) {
        window.console.log("Close the web socket connection if there is one");
        if ((client_data.ws !== null) && ((client_data.ws.readyState === window.WebSocket.CONNECTING) ||
                                          (client_data.ws.readyState === window.WebSocket.OPEN))) {
            window.console.log("Disable the controls before connecting to a new server");
            disable_interface();
            
            window.console.log("Closing the previously opened connection");
            update_conn_status(window.WebSocket.CLOSING);
            client_data.ws.close();
        }

        window.console.log("Create a new web socket to: " + client_data.server_url);
        client_data.ws = new window.WebSocket(client_data.server_url);

        update_conn_status(window.WebSocket.CONNECTING);

        window.console.log("Set up the socket handler functions");
        client_data.ws.onopen = on_open;
        client_data.ws.onmessage = on_message;
        client_data.ws.onclose = on_close;
    } else {
        //Disable the web page
        error("The WebSockets are not supported by your browser!");
    }
}

/**
 * This function is called if the server URL change event is fired,
 * so we need to check if we need to (re-)connect.
 */
function on_server_change() {
    "use strict";
    var new_server_url;
    
    //Get the current value and trim it
    new_server_url = client_data.server_url_inpt.value.trim();
    //Put the trimmed value back into the input
    client_data.server_url_inpt.value = new_server_url;
    
    window.console.log("The new server url is: " + new_server_url);

    if ((client_data.ws.readyState !== window.WebSocket.OPEN) ||
            (client_data.server_url !== new_server_url)) {
        
        //A new server means new translation
        require_new_translation();
        
        window.console.log("Storing the new server url value");
        client_data.server_url = new_server_url;
    
        if (client_data.server_url !== "") {
            window.console.log("Connecting to the new server url");
            connect_to_server();
        }
    } else {
        window.console.log("The server url has not changed");
    }
}

/**
 * This function initializes the client data
 */
function initialize_client_data(callMD5) {
    "use strict";
    
    window.console.log("Initial client data: " + client_data);
    
    //Get the references to the needed UI elements
    client_data.trans_btn = document.getElementById("trans_btn");
    client_data.trans_info_cb = document.getElementById("trans_info_cb");
    client_data.from_text_area = document.getElementById("from_text");
    client_data.to_text_span = document.getElementById("to_text");
    client_data.from_lang_sel = document.getElementById("from_lang_sel");
    client_data.to_lang_sel = document.getElementById("to_lang_sel");
    client_data.server_url_inpt = document.getElementById("server_url");
    client_data.progress_image = document.getElementById("progress");
    client_data.conn_status_span = document.getElementById("conn_status");
    client_data.trans_info_cb = document.getElementById("trans_info_cb");
    client_data.request_progress_bar = window.$("#request_progress_bar");
    client_data.response_progress_bar = window.$("#response_progress_bar");
    
    //Set up the server URL
    client_data.server_url_inpt.value = client_data.server_url;
    
    //Store the MD5 function
    client_data.calcMD5 = callMD5;
}

/**
 * This function is called in the beginning just after the DOM tree
 * is parsed and ensures initialization of the web interface.
 * @param {Object} callMD5 the function to compute MD5
 */
function initialize_translation(callMD5) {
    "use strict";
    
    window.console.log("Initializing the client data");
    
    //Initialize the client data
    initialize_client_data(callMD5);

    window.console.log("Open an initial connection to the server");

    //Connect to the server - open websocket connection
    connect_to_server();
}
