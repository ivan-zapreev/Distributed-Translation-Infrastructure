var client_data = {
    "server_url" : "ws://localhost:9002", //The web server url
    "ws" : null,                          //The web socket to the server
    "is_translating" : false,             //Indicates whether we are translating or not
    "sent_trans_req" : 0,                 //The number of sent translation requests
    "received_trans_resp" : 0,            //The number of received translation responses
    "job_responces" : [],                 //Stores the translation job responses
    "translation_html" : "",              //Stores the translation HTML data
    "prev_job_req_id" : 1,                //Stores the previously send translation job request id
    "prev_source_md5" : "",               //Stores the previously sent translation job source text
    "calcMD5" : null,                     //The md5 function, to be initialized
    "callDownload" : null,                //The download function, to be initialized
    "server_url_inpt" : null              //Stores the server url input
};

//The communication protocol version value
var PROTOCOL_VERSION = 1;
//The maximum number of sentences per translation job request.
var MAX_NUM_SENTENCES_PER_JOB = 128;
//Stores the number of progress bars for the translation process
var NUM_PROGRESS_BARS = 2;
//Store the undefined job id value
var UNDEFINED_JOB_ID = 0;
//Stores the minimum translation job id
var MINIMUM_TRANS_JOB_ID = 1;
//Stores the number of bytes in one Mb
var NUMBER_OF_BYTES_IN_MEGABYTE = 1024 * 1024;
//Stores the maximum file limit for the file upload
var MAXIMUM_FILE_UPLOAD_SIZE_MB = 3; //Maximum 3mb in bytes
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
    RESULT_PARTIAL : 3,
    RESULT_CANCELED : 4,
    RESULT_ERROR : 5
};

//Declare the supported languages request
var SUPPORTED_LANG_REQ = {"prot_ver" : PROTOCOL_VERSION, "msg_type" : MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_REQ};
//Declare the supported languages request
var TRAN_JOB_REQ_BASE = {"prot_ver" : PROTOCOL_VERSION, "msg_type" : MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_REQ};

//Declare the "Please select" string for the srouce/target language select boxes
var PLEASE_SELECT_STRING = "Please select";

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
        client_data.trans_server_cs_img.attr("src", "./img/changing.png");
        client_data.trans_server_cs_img.attr("alt", "Connecting ...");
        client_data.trans_server_cs_img.attr("data-original-title", "Connecting ...");
        break;
    case window.WebSocket.OPEN:
        client_data.trans_server_cs_img.attr("src", "./img/connected.png");
        client_data.trans_server_cs_img.attr("alt", "Connected");
        client_data.trans_server_cs_img.attr("data-original-title", "Connected");
        break;
    case window.WebSocket.CLOSING:
        client_data.trans_server_cs_img.attr("src", "./img/changing.png");
        client_data.trans_server_cs_img.attr("alt", "Disconnecting ...");
        client_data.trans_server_cs_img.attr("data-original-title", "Disconnecting ...");
        break;
    case window.WebSocket.CLOSED:
        client_data.trans_server_cs_img.attr("src", "./img/disconnected.png");
        client_data.trans_server_cs_img.attr("alt", "Disconnected");
        client_data.trans_server_cs_img.attr("data-original-title", "Disconnected");
        break;
    default:
        client_data.trans_server_cs_img.attr("src", "./img/puzzled.png");
        client_data.trans_server_cs_img.attr("alt", "Puzzled :)");
        client_data.trans_server_cs_img.attr("data-original-title", "Puzzled :)");
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
 * ALlows to change the badge value
 * @param {Jquery Object} badge the badge object to work with
 * @param {Boolean} is_set if true then the value will be set, otherwise added
 * @param {Number} factor the factor to add/set to the badge value
 */
function change_badge_value(badge, is_set, factor) {
    "use strict";
    
    var value;
    
    if (is_set) {
        //Set the badge value
        badge.html(factor);
    } else {
        //Increment the badge value
        value = window.parseInt(badge.html());
        window.console.log("The badge value is: " + value);
        badge.html(value + factor);
    }
}

/**
 * Allows to clear the log panel and re-set the badges
 */
function do_clear_log() {
    "use strict";
    
    //Clear the log panel
    client_data.log_panel.html("");
    //Re-set the badges
    change_badge_value(client_data.lp_dang, true, 0);
    change_badge_value(client_data.lp_warn, true, 0);
    change_badge_value(client_data.lp_info, true, 0);
    change_badge_value(client_data.lp_succ, true, 0);
}

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
 * Allows to add a message of a given type to log
 * @param {Json Object} the badge object to get the data set into
 * @param {String} type the log entry type
 * @param {String} message the message to be placed
 * @param {Boolean} is_alert true if an alert box is to be show, otherwise false, optional, default is false
 */
function add_log_message(badge, type, message, is_alert) {
    "use strict";
    
    is_alert = is_alert || false;
    
    var value;
    
    //Increment the number of messages of the given sort
    change_badge_value(badge, false, +1);
    
    client_data.log_panel.append(
        "<div class='alert alert-" + type + " fade in'>" + get_date_string() +
            " | <strong>" + type + ":</strong> " + escape_html(message) + "</div>"
    );
    
    //Scroll down to see let one see the result
    //client_data.log_panel.scrollTop(client_data.log_panel.prop("scrollHeight"));
    
    //Do the alert if it is needed
    if (is_alert) {
        window.alert(message);
    }
}

/**
 * This function is called in case an error message is to be logged.
 * @param {String} message the message to visualize
 * @param {Boolean} is_alert true if an alert box is to be show, otherwise false, optional, default is false
 */
function danger(message, is_alert) {
    "use strict";
    
    is_alert = is_alert || false;
    
    add_log_message(client_data.lp_dang, "danger", message, is_alert);
    
    window.console.error(message);
}

/**
 * This function is called in case a warning message is to be logged
 * @param {String} message the message to visualize
 */
function warning(message, is_alert) {
    "use strict";
    
    is_alert = is_alert || false;
    
    add_log_message(client_data.lp_warn, "warning", message, is_alert);

    window.console.warn(message);
}

/**
 * This function is called in case an info message is to be logged
 * @param {String} message the message to visualize
 */
function info(message, is_alert) {
    "use strict";
    
    is_alert = is_alert || false;
    
    add_log_message(client_data.lp_info, "info", message, is_alert);

    window.console.log(message);
}

/**
 * This function is called in case a success message is to be logged.
 * @param {String} message the message to visualize
 */
function success(message) {
    "use strict";
    
    add_log_message(client_data.lp_succ, "success", message);
    
    window.console.info(message);
}

/**
 * Allows to visualize the status code and message if needed
 * @param {Number} the job id 
 * @param {Number} stat_code the job response status code
 * @param {String} stat_msg the status message string the status message
 */
function visualize_status_code(job_id, stat_code, stat_msg) {
    "use strict";
    
    //Keen the maximum status code as the higher the more cevere is the error
    var trans_status = window.Math.max(stat_code, client_data.trans_status);
    
    //Log the event
    switch (stat_code) {
    case STATUS_CODE_ENUM.RESULT_OK:
        success("Translation job: " + job_id + " succeeded!");
        break;
    case STATUS_CODE_ENUM.RESULT_ERROR:
        danger("Translation job: " + job_id + " failed: " + stat_msg);
        break;
    case STATUS_CODE_ENUM.RESULT_CANCELED:
        warning("Translation job: " + job_id + " was cancelled: " + stat_msg);
        break;
    case STATUS_CODE_ENUM.RESULT_PARTIAL:
        warning("Translation job: " + job_id + " was partially done: " + stat_msg);
        break;
    default:
        break;
    }
    
    //Check if we need to change the visualization
    if (client_data.trans_status !== trans_status) {
        //Keep the new status
        client_data.trans_status = trans_status;
        //Visualize
        switch (trans_status) {
        case STATUS_CODE_ENUM.RESULT_OK:
            client_data.to_text_span.css("box-shadow", "0 0 10px green");
            break;
        case STATUS_CODE_ENUM.RESULT_ERROR:
            client_data.to_text_span.css("box-shadow", "0 0 10px red");
            break;
        case STATUS_CODE_ENUM.RESULT_CANCELED:
            client_data.to_text_span.css("box-shadow", "0 0 10px orange");
            break;
        case STATUS_CODE_ENUM.RESULT_PARTIAL:
            client_data.to_text_span.css("box-shadow", "0 0 10px yellow");
            break;
        default:
            break;
        }
    }
}

/**
 * Allows to re-set the status code visualization
 */
function remove_status_code_visual() {
    "use strict";
    
    //Re-set the stored status and remove the visual effect
    client_data.trans_status = STATUS_CODE_ENUM.RESULT_UNDEFINED;
    client_data.to_text_span.css("box-shadow", "none");
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
    client_data.input_file_select.prop("disabled", true);
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
        client_data.input_file_select.prop("disabled", false);
    }
}

/**
 * Allows to set the progress bar progress
 * @param {Boolean} is_init true if this is for (re-)initialization of the progress bar
 * @param {jquery Object} the jquesy object of the progress bar
 * @param {String} the message to be visualized
 * @param {Number} the current value for the progress bar
 * @param {Number} the maximum value for the progress bar
 * @param {Number} the number of translation process progress bars
 */
function set_progress_bar(is_init, pb, msg, curr_num, max_num, num_prog_bars) {
    "use strict";
    
    window.console.log("The current value: " + curr_num + " max value: " + max_num);
    
    var percent = window.Math.round((curr_num / max_num) * 100), span = pb.find("span");
    pb.attr("aria-valuenow", percent);
    pb.width((percent / NUM_PROGRESS_BARS) + "%");

    if (is_init) {
        span.html("");
    } else {
        span.html(escape_html(msg + ": " + percent + "%"));
    }
    
    if (percent === 0 && !is_init) {
        pb.addClass("active");
    }
    if (percent === 100 || is_init) {
        pb.removeClass("active");
    }
}

/**
 * Allows to re-initialize the request/response progress bars
 */
function initialize_progress_bars() {
    "use strict";

    set_progress_bar(true, client_data.response_progress_bar, "Responses", 0, 1);
    set_progress_bar(true, client_data.request_progress_bar, "Requests", 0, 1);
}

/**
 * Allows to set the new value for the translation responses progress bar
 * @param {integer} curr the current value
 * @param {integer} max the maximum value
 */
function set_response_progress_bar(curr, max) {
    "use strict";
    
    set_progress_bar(false, client_data.response_progress_bar, "Responses", curr, max);
}

/**
 * Allows to set the new value for the translation requests progress bar
 * @param {integer} curr the current value
 * @param {integer} max the maximum value
 */
function set_request_progress_bar(curr, max) {
    "use strict";
    
    set_progress_bar(false, client_data.request_progress_bar, "Requests", curr, max);
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
        client_data.progress_image.src = "./img/globe32.png";
    } else {
        client_data.progress_image.src = "./img/globe32.gif";
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
    visualize_status_code(trans_response.job_id, trans_response.stat_code, trans_response.stat_msg);
    
    //Allow a new translation in case the job status code is NOT
    if (trans_response.stat_code !== STATUS_CODE_ENUM.RESULT_OK) {
        require_new_translation();
    }

    //Only visualize the results if the target data is present
    if (trans_response.hasOwnProperty('target_data')) {
        //Assemble the data
        for (j = 0; j < trans_response.target_data.length; j += 1) {
            //Get the target
            target = trans_response.target_data[j];
            //Create the status string, to hover
            status = get_status_code_string(target.stat_code);
            
            //Add the message if it is not empty
            if (target.stat_code !== STATUS_CODE_ENUM.RESULT_OK) {
                status += ": " + target.stat_msg;
                switch (target.stat_code) {
                case STATUS_CODE_ENUM.RESULT_ERROR:
                    if (target.stat_msg !== "") {
                        danger("Translation job " + trans_response.job_id + ", sentence " + (j + 1) + " failed: " + target.stat_msg);
                    }
                    break;
                case STATUS_CODE_ENUM.RESULT_CANCELED:
                    if (trans_response.stat_code !== STATUS_CODE_ENUM.RESULT_CANCELED) {
                        if (target.stat_msg !== "") {
                            warning("Translation job " + trans_response.job_id + ", sentence " + (j + 1) + " cancelled: " + target.stat_msg);
                        }
                    }
                    break;
                }
            }
            
            //Check if the stack loads are present, if yes, add them
            if (target.hasOwnProperty('stack_load')) {
                status += ", multi-stack loads(%): [" + target.stack_load.join(" ") + "]";
            } else {
                if (client_data.trans_info_cb.checked) {
                    window.console.warn("The stack_load field is not present when the translation infos are requested!");
                }
            }
            
            //Add the translation element to the panel
            client_data.translation_html += "<span class='target_sent_tag' title='' data-original-title='" +
                escape_html(status) + "' data-toggle='tooltip' data-placement='auto'>" + escape_html(target.trans_text) + "</span>";
        }
    } else {
        window.console.warn("The target_data field is not present in the translation response!");
    }
    
    //Check if this is the last response
    if (response_idx === (trans_responses.length - 1)) {
        //Set the translation html into the DOM tree just once, otherwise it is too slow!
        client_data.to_text_span.html(client_data.translation_html);
        window.$('[data-toggle="tooltip"]').tooltip();
        client_data.translation_html = "";
        
        //Update the translation status
        update_trans_status();

        //Enable the interface controls, in case the status
        //is connected. This method is executed asynchronously
        //sand thus may be the connection is already lost
        enable_interface(client_data.ws.readyState === window.WebSocket.OPEN);
        
        //Notify the user that everything went fine
        success("Finished recombining " + trans_responses.length + " translation responses!");
    }
}

/**
 * Allows to process a large array in an asynchronous way
 * @param {integer} min_idx the minimum index value
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

    info("Start re-combining " + array.length + " translation response(s)");
    
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
        client_data.is_translating = false;

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

        //Remove the visualization of the status
        remove_status_code_visual();
    }
    
    //Increment the number of active translations
    client_data.sent_trans_req += 1;
    window.console.log("A new request, #jobs: " + client_data.sent_trans_req);

    //Update the translation status
    update_trans_status();
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
 * Allows to check if a language is selected
 * @param {Object} the select element to get the selected value from
 * @param {Boolean} true if a language is selected, otherwise false
 */
function is_language_selected(select) {
    "use strict";
    
    //Get the selected language
    var selected = get_selected_lang(select);
    
    //The selected value must not be empty
    return (selected !== "");
}

/**
 * Allows to check if the source language is selected
 * @param {Boolean} true if a language is selected, otherwise false
 */
function is_source_lang_selected() {
    "use strict";

    return is_language_selected(client_data.from_lang_sel);
}

/**
 * Allows to check if the target language is selected
 * @param {Boolean} true if a language is selected, otherwise false
 */
function is_target_lang_selected() {
    "use strict";

    return is_language_selected(client_data.to_lang_sel);
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
            client_data.prev_job_req_id = UNDEFINED_JOB_ID;

            //Re-set the job responses array
            client_data.job_responces = [];

            //Check if the source language is selected
            if (is_source_lang_selected()) {
                //Set the source language
                source_lang = get_selected_source_lang();
                
                //Check if the target language is selected
                if (is_target_lang_selected()) {
                    //Set the target language
                    target_lang = get_selected_target_lang();

                    //Get the translation info flag from thecheckox!
                    is_trans_info = client_data.trans_info_cb.checked;

                    //Clear the current translation text
                    client_data.to_text_span.html("");

                    sent_array = source_text.split('\n');
                    window.console.log("Send the translation requests for " + sent_array.length + " sentences");

                    //Set the translating flag
                    client_data.is_translating = true;
                    
                    //Re-initialize the progress bars
                    initialize_progress_bars();

                    begin_idx = 0;
                    while (begin_idx < sent_array.length) {
                        //Compute the end index
                        end_idx = window.Math.min(begin_idx + MAX_NUM_SENTENCES_PER_JOB, sent_array.length);
                        window.console.log("Sending sentences [" + begin_idx + "," + end_idx + ")");
                        send_translation_request(source_lang, target_lang, is_trans_info, sent_array.slice(begin_idx, end_idx));
                        begin_idx = end_idx;
                        set_request_progress_bar(end_idx, sent_array.length);
                    }

                    //Make the progress note visible
                    set_response_progress_bar(0, 1);

                    success("Sent out " + client_data.sent_trans_req + " translation requests");
                } else {
                    danger("Please selecte the target language!", true);
                }
            } else {
                danger("Please selecte the source language!", true);
            }
            
            window.console.log("Finished sending translation request jobs.");
        } else {
            warning("This translation job has already been done!", true);
        }
    } else {
        warning("There is no text to translate!", true);
    }
}

/**
 * This function is called when a connection with the translation server is open.
 */
function on_open() {
    "use strict";
    
    success("The connection to '" + client_data.server_url + "' is open");
    update_conn_status(window.WebSocket.OPEN);

    //Sent the request for the supported languages
    var supp_lang_reg_str = JSON.stringify(SUPPORTED_LANG_REQ);
    window.console.log("Sending the supported languages request: " + supp_lang_reg_str);
    info("Requesting supported languages from the server!");
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
    client_data.job_responces[trans_response.job_id - MINIMUM_TRANS_JOB_ID] = trans_response;

    //Log that we received the response into the console
    info("Received response for a translation job, id: " + trans_response.job_id);
    
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

    success("Received a supported languages response from the server!");
    
    //Store the supported languages
    client_data.language_mapping = supp_lang_resp.langs;
    
    //Filter the source languages without the targets
    //It can happen that for the soure language there is no targets, then ignore it.
    //We do it here and not on the server as here it is easier to check and on the
    //server it will require extra computation effort and will complicate the code.
    window.console.log("Filter the empty source languages");
    for (source_lang in client_data.language_mapping) {
        if (client_data.language_mapping.hasOwnProperty(source_lang)) {
            if (client_data.language_mapping[source_lang].length === 0) {
                delete client_data.language_mapping[source_lang];
            }
        }
    }
    
    window.console.log("Clear the to-select as we are now re-loading the source languages");
    client_data.to_lang_sel.innerHTML = get_select_option("", PLEASE_SELECT_STRING);

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
    
    //Check if the message type is detectable
    if (resp_obj.hasOwnProperty('msg_type')) {
        //Check of the message type
        if (resp_obj.msg_type === MSG_TYPE_ENUM.MESSAGE_TRANS_JOB_RESP) {
            set_translation(resp_obj);
        } else {
            if (resp_obj.msg_type === MSG_TYPE_ENUM.MESSAGE_SUPP_LANG_RESP) {
                set_supported_languages(resp_obj);
            } else {
                danger("An unknown server message type: " + evt.data);
            }
        }
    } else {
        danger("Malformed server message: " + evt.data);
    }
}

/**
 * This function is called when a connection with the translation server is dropped.
 */
function on_close() {
    "use strict";
    
    if (client_data.is_translating) {
        danger("Falied to perform translation the server dropped off!", true);
        //Set the translating flag back to false
        client_data.is_translating = false;
        //Re-initialize the progress bars
        initialize_progress_bars();
    } else {
        warning("The connection to '" + client_data.server_url + "' is closed");
    }
    
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

    window.console.log("Disable the controls before connecting to a new server");
    disable_interface();

    try {
        window.console.log("Checking that the web socket connection is available");
        if (window.hasOwnProperty("WebSocket")) {
            window.console.log("Close the web socket connection if there is one");
            if ((client_data.ws !== null) && ((client_data.ws.readyState === window.WebSocket.CONNECTING) ||
                                              (client_data.ws.readyState === window.WebSocket.OPEN))) {
                info("Closing the previously opened connection");
                update_conn_status(window.WebSocket.CLOSING);
                client_data.ws.close();
            }

            info("Opening a new web socket to the server: " + client_data.server_url);
            client_data.ws = new window.WebSocket(client_data.server_url);

            update_conn_status(window.WebSocket.CONNECTING);

            window.console.log("Set up the socket handler functions");
            client_data.ws.onopen = on_open;
            client_data.ws.onmessage = on_message;
            client_data.ws.onclose = on_close;
        } else {
            //Disable the web page
            danger("The WebSockets are not supported by your browser!");
        }
    } catch (err) {
        enable_interface(false);
    }
}

/**
 * This function is called if the server URL change event is fired,
 * so we need to check if we need to (re-)connect.
 */
function on_server_change() {
    "use strict";
    var server_url;
    
    //Get the current value and trim it
    server_url = client_data.server_url_inpt.value.trim();
    //Put the trimmed value back into the input
    client_data.server_url_inpt.value = server_url;
    
    window.console.log("The new server url is: " + server_url);

    if ((client_data.ws.readyState !== window.WebSocket.OPEN) ||
            (client_data.server_url !== server_url)) {
        
        //A new server means new translation
        require_new_translation();
        
        window.console.log("Storing the new server url value");
        client_data.server_url = server_url;
    
        if (client_data.server_url !== "") {
            window.console.log("Connecting to the new server url");
            connect_to_server();
        }
    } else {
        window.console.log("The server url has not changed");
    }
}

/**
 * This method should handle the finish of file reading event
 * @param {Object} evt the event that the file has been read
 */
function on_input_file_read(evt) {
    "use strict";

    success("The file is loaded!");
    
    client_data.from_text_area.value = evt.target.result;
}

/**
 * The function that will be called once the upload file is selected
 * @param {Object} evt the change event
 */
function on_upload_file_select(evt) {
    "use strict";
    
    var file, reader, size_mb;
    
    //Get the file
    file = evt.target.files[0];
    size_mb = file.size / NUMBER_OF_BYTES_IN_MEGABYTE;
    size_mb = window.parseFloat(size_mb.toFixed(1));
    info("Selected a file to translate: " + file.name + ", " + size_mb + " Mb");
    
    if (size_mb <= MAXIMUM_FILE_UPLOAD_SIZE_MB) {
        //Read the file into the text field
        reader = new window.FileReader();
        reader.onload = on_input_file_read;

        info("Start loading the file into memory!");
        reader.readAsText(file, 'UTF-8');
    } else {
        //Clear the file input by replacing its container's html content 
        client_data.input_file_sc.html(client_data.input_file_sc.html());
        client_data.input_file_select = window.$("#input_file_select");
        client_data.input_file_select.bind('change', on_upload_file_select);
        danger("The file '" + file.name + "' size is " + size_mb +
               " Mb, the maximum we allow is " + MAXIMUM_FILE_UPLOAD_SIZE_MB +
               " Mb!", true);
    }
}

/**
 * Obtains the element references
 */
function obtain_element_references() {
    "use strict";
    
    //Get the references to the needed UI elements
    client_data.trans_btn = document.getElementById("trans_btn");
    client_data.trans_info_cb = document.getElementById("trans_info_cb");
    client_data.from_text_area = document.getElementById("from_text");
    client_data.from_lang_sel = document.getElementById("from_lang_sel");
    client_data.to_lang_sel = document.getElementById("to_lang_sel");
    client_data.server_url_inpt = document.getElementById("trans_server_url");
    client_data.progress_image = document.getElementById("progress");
    client_data.trans_info_cb = document.getElementById("trans_info_cb");
    client_data.clear_log_btn = document.getElementById("log_clear_btn");
    
    client_data.to_text_span = window.$("#to_text");
    client_data.trans_server_cs_img = window.$("#trans_server_cs");
    client_data.request_progress_bar = window.$("#request_progress_bar");
    client_data.response_progress_bar = window.$("#response_progress_bar");
    client_data.log_panel = window.$("#log_panel");
    client_data.lp_dang = window.$("#lp_dang");
    client_data.lp_warn = window.$("#lp_warn");
    client_data.lp_info = window.$("#lp_info");
    client_data.lp_succ = window.$("#lp_succ");
    client_data.input_file_select = window.$("#input_file_select");
    client_data.input_file_sc = window.$("#input_file_sc");
    client_data.download_text_link = window.$("#download_text_link");
    client_data.download_log_link = window.$("#download_log_link");
}

/**
 * Will be called when a text download link is clicked
 * @param {Object} the on click event 
 */
function download_text_translation(evt) {
    "use strict";
    
    var file_name, text;
    
    //Construct the file name and do logging
    file_name = "translation." + get_date_string('.') + ".txt";
    window.console.log("Downloading: " + file_name);
    
    //Get the translations.
    text = "";
    window.$(".target_sent_tag").each(function (index) {
        text += window.$(this).text() + "\n";
    });

    window.console.log("Text: " + text);

    //Call the download function from the library
    client_data.callDownload(text, file_name, "text/html; charset=UTF-8");
    
    //Reporta a warning
    if (client_data.sent_trans_req !== client_data.received_trans_resp) {
        warning("Download file: The translation process is not finished!");
    }
}

/**
 * Will be called when a log download link is clicked
 * @param {Object} the on click event 
 */
function download_log_translation(evt) {
    "use strict";
    
    var file_name, text;
    
    //Construct the file name and do logging
    file_name = "translation." + get_date_string('.') + ".log";
    window.console.log("Downloading: " + file_name);
    
    //Get the translations.
    text = "";
    window.$(".target_sent_tag").each(function (index) {
        text += window.$(this).attr("data-original-title") + "\n";
    });

    window.console.log("Text: " + text);

    //Call the download function from the library
    client_data.callDownload(text, file_name, "text/html; charset=UTF-8");
    
    //Reporta a warning
    if (client_data.sent_trans_req !== client_data.received_trans_resp) {
        warning("Download file: The translation process is not finished!");
    }
}

/**
 * This function initializes the client data
 * @param {Object} callMD5 the function to compute MD5
 * @param {Object} callDownload the function to download file from JavaScript
 */
function initialize_client_data(callMD5, callDownload) {
    "use strict";
    
    //Initialize tooltips
    window.$('[data-toggle="tooltip"]').tooltip();
    
    window.console.log("Using the default translation server url: " + client_data.server_url);
    
    //Set up the server URL
    client_data.server_url_inpt.value = client_data.server_url;
    
    //Store the MD5 function
    client_data.calcMD5 = callMD5;
    
    //Store the Download function
    client_data.callDownload = callDownload;
    
    //Re-set progress bars
    initialize_progress_bars();
    
    //Check for the various File API support.
    if (window.File && window.FileReader && window.FileList && window.Blob) {
        client_data.is_file_support = true;
        //Add the selection fuinction handler
        client_data.input_file_select.bind('change', on_upload_file_select);
    } else {
        client_data.is_file_support = false;
        warning('The File APIs are not (fully) supported in this browser.');
        //Disable the file upload related elements
        client_data.input_file_select.hide();
    }
    
    //Add the link download handlers
    client_data.download_text_link.click(download_text_translation);
    client_data.download_log_link.click(download_log_translation);
}

/**
 * This function is called in the beginning just after the DOM tree
 * is parsed and ensures initialization of the web interface.
 * @param {Object} callMD5 the function to compute MD5
 * @param {Object} callDownload the function to download file from JavaScript
 */
function initialize_translation(callMD5, callDownload) {
    "use strict";
    
    //Get the references to needed UI elements
    obtain_element_references();
    
    //Initialize the client data
    initialize_client_data(callMD5, callDownload);

    window.console.log("Open an initial connection to the translation server");

    //Connect to the server - open websocket connection
    connect_to_server();
}
