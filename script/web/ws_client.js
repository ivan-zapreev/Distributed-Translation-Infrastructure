/*
 *  File:   ws_client.js
 *  Author: Dr. Ivan S. Zapreev
 *  Visit my Linked-in profile:
 *       <https://nl.linkedin.com/in/zapreevis>
 *  Visit my GitHub:
 *       <https://github.com/ivan-zapreev>
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  Created on November 14, 2016, 11:07 AM
 */

//Stores the number of progress bars for the translation process
var NUM_PROGRESS_BARS = 2;

/**
 * Allows to create a new web socket server client
 * @param common_mdl {Object} the common module
 * @param url_input {Object} the url input dom object
 * @param init_url {String} the server's initial url to work with
 * @param server_cs_img {Object} the Dom object of the server connection status image
 * @param server_cs_bage {Object} the Dom object of the server connection status badge  
 * @param on_open_fn {Function} the function to call when the connection is open
 * @param on_message_fn {Function} the function to call when a messaage is received 
 * @param on_close_fn {Function} the function to call when the connection is closed
 * @param request_pb {Object} the Dom object of the requests progress bar
 * @param response_pb {Object} the Dom object of the responses progress bar
 * @return the web socket client module
 */
function create_ws_client(common_mdl, url_input, init_url,
                          server_cs_img, server_cs_bage,
                          on_open_fn, on_message_fn, on_close_fn,
                          request_pb, response_pb) {
    "use strict";
    
    //Declare the variables
    var is_req_focus_on_enable, is_requested_close, client;
    
    //Check if the initial url has to be changed to the stored one
    init_url = common_mdl.get_cookie_fn(url_input.attr('id'), init_url);
    
    //Create the first prototype of the client module
    client = {
        PROTOCOL_VERSION : 1,
        MSG_TYPE_ENUM : {
            //The message type is undefined
            MESSAGE_UNDEFINED : 0,
            //The supported languages request message
            MESSAGE_SUPP_LANG_REQ : 1,
            //The supported languages response message
            MESSAGE_SUPP_LANG_RESP : 2,
            //The translation job request message
            MESSAGE_TRANS_JOB_REQ : 3,
            //The translation job response message
            MESSAGE_TRANS_JOB_RESP : 4,
            //The pre-processor job request message
            MESSAGE_PRE_PROC_JOB_REQ : 5,
            //The pre-processor job response message
            MESSAGE_PRE_PROC_JOB_RESP : 6,
            //The post-processor job request message
            MESSAGE_POST_PROC_JOB_REQ : 7,
            //The post-processor job response message
            MESSAGE_POST_PROC_JOB_RESP : 8
        },
        url : init_url,          //The web server url
        ws : null                //The web socket to the server
    };
    
    //Set the focus needed flag to false
    is_req_focus_on_enable = false;
    
    //Set the close requested flag to false
    is_requested_close = false;
    
    window.console.log("Creating a new ws client for the server: " + client.url);
    
    //Set the url into the server input, but first see
    //if there is already one stored in the cookie
    url_input.val(init_url);

    /**
     * Allows to process large data in an asynchronous way
     * @param {integer} min_idx the minimum index value
     * @param {data} the data to process , must have the length property
     * @param {function} the call back function to be called per array element
     * @param {time} the time allowed to be busy per batch, optional
     * @param {context} context the context, optional
     */
    function process_data_async(data, fn, maxTimePerChunk, context) {
        context = context || window;
        maxTimePerChunk = maxTimePerChunk || 200;
        var index = 0;

        common_mdl.logger_mdl.info("Start processing " + data.length + " elements");

        function now() {
            var time = new Date().getTime();
            window.console.log("Next iteration time is: " + time);
            return time;
        }

        function doChunk() {
            var startTime = now();
            while (index < data.length && (now() - startTime) <= maxTimePerChunk) {
                // callback called with args (value, index, data)
                fn.call(context, data, index);
                index += 1;
            }
            if (index < data.length) {
                // set Timeout for async iteration
                setTimeout(doChunk, 1);
            }
        }
        doChunk();
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
        window.console.log("The current value: " + curr_num + " max value: " + max_num);

        var percent = window.Math.round((curr_num / max_num) * 100), span = pb.find("span");
        pb.attr("aria-valuenow", percent);
        pb.width((percent / NUM_PROGRESS_BARS) + "%");

        if (is_init) {
            span.html("");
        } else {
            span.html(common_mdl.escape_html_fn(msg + ": " + percent + "%"));
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
        window.console.log("Initializing progress bards!");
        set_progress_bar(true, response_pb, "Responses", 0, 100);
        set_progress_bar(true, request_pb, "Requests", 0, 100);
    }

    /**
     * Allows to set the new value for the translation responses progress bar
     * @param {integer} curr the current value
     * @param {integer} max the maximum value
     */
    function set_response_pb(curr, max) {
        set_progress_bar(false, response_pb, "Responses", curr, max);
    }

    /**
     * Allows to set the new value for the translation requests progress bar
     * @param {integer} curr the current value
     * @param {integer} max the maximum value
     */
    function set_request_pb(curr, max) {
        set_progress_bar(false, request_pb, "Requests", curr, max);
    }
    
    //Export the request/response progress bar setters, initiaalizers
    client.set_response_pb_fn = set_response_pb;
    client.set_request_pb_fn = set_request_pb;
    client.init_req_resp_pb_fn = initialize_progress_bars;

    /**
     * This function allows to update the current connection status
     * in the GUI. We use the same values as the Websocket connection.
     * @param {Number} ws_status the Webscoket status value to be stored
     */
    function update_conn_status(ws_status) {
        switch (ws_status) {
        case window.WebSocket.CONNECTING:
            server_cs_img.attr("src", "./img/changing.gif");
            server_cs_img.attr("alt", "Connecting ...");
            server_cs_img.attr("data-original-title", "Connecting ...");
            server_cs_bage.attr("class", "badge changing");
            break;
        case window.WebSocket.OPEN:
            server_cs_img.attr("src", "./img/connected.png");
            server_cs_img.attr("alt", "Connected");
            server_cs_img.attr("data-original-title", "Connected");
            server_cs_bage.attr("class", "badge online");
            break;
        case window.WebSocket.CLOSING:
            server_cs_img.attr("src", "./img/changing.gif");
            server_cs_img.attr("alt", "Disconnecting ...");
            server_cs_img.attr("data-original-title", "Disconnecting ...");
            server_cs_bage.attr("class", "badge changing");
            break;
        case window.WebSocket.CLOSED:
            server_cs_img.attr("src", "./img/disconnected.png");
            server_cs_img.attr("alt", "Disconnected");
            server_cs_img.attr("data-original-title", "Disconnected");
            server_cs_bage.attr("class", "badge offline");
            break;
        default:
            server_cs_img.attr("src", "./img/puzzled.png");
            server_cs_img.attr("alt", "Puzzled :)");
            server_cs_img.attr("data-original-title", "Puzzled :)");
            server_cs_bage.attr("class", "badge puzzled");
            break;
        }
    }

    /**
     * This function allows to enable the user interface and re-set the input focus back if needed.
     */
    function enable_interface() {
        window.console.log("Re-enabling the user interface");
        
        //Enable the interface controls
        common_mdl.enable_interface_fn();
        
        window.console.log("Checking if we need to set url input focus: " + is_req_focus_on_enable);

        if (is_req_focus_on_enable) {
            window.console.log("Re-setting the focus on the url input element");

            //Make sure that the input gets focus and select
            url_input.focus().select();
        }

        //Always set the focus-required flag (back) to disabled
        is_req_focus_on_enable = false;
    }
    
    /**
     * The function that allows to open a new connection the the WS server
     * @param url {String} the url to connec to
     */
    function open_new_connection(url) {
        //The soccet was not requested to be closed or is already closed
        window.console.log("Storing the new server url value: " + url);
        client.url = url;

        common_mdl.logger_mdl.info("Opening a new web socket to the server: " + client.url);
        client.ws = new window.WebSocket(client.url);

        update_conn_status(window.WebSocket.CONNECTING);

        window.console.log("Set up the socket handler functions");

        //Set the on connection open handler
        client.ws.onopen = function () {
            common_mdl.logger_mdl.success("The connection to '" + client.url + "' is open");
            update_conn_status(window.WebSocket.OPEN);

            //Call the on open connection handler
            if (on_open_fn) {
                on_open_fn();
            } else {
                window.console.warn("The on-open handler for server " + client.url + " is not set!");
            }

            //Enable the interface controls
            enable_interface();
        };

        //Set the on message handler
        client.ws.onmessage = function (evt) {
            //log the json data
            window.console.log("Received message: " + evt.data);

            try {
                window.console.log("Parsing to JSON");
                var resp_obj = JSON.parse(evt.data);

                //Check if the message type is detectable
                if (resp_obj.hasOwnProperty('msg_type')) {
                    //Call the on message handler with the json object
                    if (on_message_fn) {
                        on_message_fn(resp_obj);
                    } else {
                        window.console.warn("The on-message handler for server " +
                                            client.url + " is not set!");
                    }
                } else {
                    common_mdl.logger_mdl.danger("Malformed server message: " + evt.data);
                }
            } catch (err) {
                //If the status is not OK then report an error and stop
                common_mdl.process_stop_fn(true, "Could nog parse JSON: '" + evt.data +
                                           "', error: '" + err.message +
                                           "', please re-load the interface!");
            }
        };

        //Set the on connection close handler
        client.ws.onclose = function () {
            update_conn_status(window.WebSocket.CLOSED);

            //Check if we requested the connection to be closed
            if (is_requested_close) {
                window.console.warn("The connection to " + client.url + " is closed");

                //Call the on closed connection handler
                if (on_close_fn) {
                    on_close_fn();
                } else {
                    window.console.warn("The on-close handler for server " +
                                        client.url + " is not set!");
                }
                is_requested_close = false;
            }

            //Re-initialize the progress bars
            initialize_progress_bars();

            //Enable the interface controls
            enable_interface();
        };
    }
    
    /**
     * Allows to connect to the new websocket only after the previous connection was closed
     * @param url {String} the url to connect to
     */
    function reconnect_when_closed(url) {
        /**
         * The waiting function set on timer.
         */
        function wait_closed() {
            //Check if we are requesting the close of the socket
            if (is_requested_close) {
                //Set Timeout for async iteration
                setTimeout(wait_closed, 1);
            } else {
                open_new_connection(url);
            }
        }
        wait_closed();
    }

    /**
     * This function is called if one needs to connect or re-connect to the translation server.
     * @param new_url {String} the new url to connect to or nothing, the default is client.url
     */
    function connect_to_server(new_url) {
        window.console.log("Disable the controls before connecting to a new server");
        common_mdl.disable_interface_fn();
        
        //Use the new url if provided or the current one
        var url = new_url || client.url;

        try {
            window.console.log("Checking that the web socket connection is available");
            if (window.hasOwnProperty("WebSocket")) {
                window.console.log("Check if we are currently connected.");
                if ((client.ws !== null) && ((client.ws.readyState === window.WebSocket.CONNECTING) ||
                                            (client.ws.readyState === window.WebSocket.OPEN))) {
                    common_mdl.logger_mdl.info("Closing the previously opened connection to: " + client.url);
                    update_conn_status(window.WebSocket.CLOSING);
                    is_requested_close = true;
                    client.ws.close();
                }

                //Re-connect to the another server then whis connection is cosed
                reconnect_when_closed(url);
            } else {
                //Disable the web page
                common_mdl.logger_mdl.danger("The WebSockets are not supported by your browser!");
            }
        } catch (err) {
            enable_interface();
        }
    }
    
    /**
     * Allows to send the job request to the server
     * @param job_req {Object} the JSON format jov request to be sent to the server
     */
    function send_request_to_server(job_req) {
        var data = JSON.stringify(job_req);
        
        window.console.log("Sending the server request: " + data);
        
        //Send a new job request
        client.ws.send(data);
    }
    
    /**
     * This function is called if the server URL change event is fired,
     * so we need to check if we need to (re-)connect.
     */
    function on_url_change(evt) {
        var new_url;
        
        window.console.log("Calling on_url_change event handler");

        //Get the current value and trim it
        new_url = url_input.val().trim();
        //Store the new value in the cookie
        common_mdl.set_cookie_fn(url_input.attr('id'), new_url);
        //Put the trimmed value back into the input
        url_input.val(new_url);

        window.console.log("The new server url is: " + new_url);

        if ((client.ws === null) || (client.ws.readyState !== window.WebSocket.OPEN) || (client.url !== new_url)) {

            //A new server means new translation
            common_mdl.needs_new_trans_fn();

            if (new_url !== "") {
                window.console.log("Connecting to the new server url: " + new_url);
                connect_to_server(new_url);
            } else {
                window.console.log("The server url is an empty string, doing nothing!");
            }
        } else {
            window.console.log("The server url: " + client.url + " did not change");
        }
    }
    
    /**
     * This function is called if the server URL focus event is fired,
     * so we need to check if we need to (re-)connect.
     */
    function on_url_focus(evt) {
        if (!is_req_focus_on_enable) {
            window.console.log("Calling on_url_focus event handler");

            //Set the flag requiring the focus to the input
            is_req_focus_on_enable = true;
            
            //Call the on url change handler
            on_url_change(evt);
        }
    }

    //Set the server change handlers
    url_input.change(on_url_change);
    url_input.focus(on_url_focus);
    
    //Set the exported functions and modules
    client.common_mdl = common_mdl;
    client.connect_fn = connect_to_server;
    client.is_connected_fn = function () {
        return ((client.ws !== null) && (client.ws.readyState === window.WebSocket.OPEN));
    };
    client.send_request_fn = send_request_to_server;
    client.process_data_async_fn = process_data_async;
    
    //Re-set progress bars
    initialize_progress_bars();

    return client;
}