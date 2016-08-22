/**
 * Allows to create a new web socket server client
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the web socket client module
 */
function create_ws_client(logger_mdl, url_input, url, server_cs_img, server_cs_bage,
                           needs_new_trans_fn, disable_interface_fn, enable_interface_fn,
                           on_open_fn, on_message_fn, on_close_fn) {
    "use strict";
    
    //Create the first prototype of the client module
    var client = {
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
            MESSAGE_TRANS_JOB_RESP : 4
        },
        STATUS_CODE_ENUM : {
            RESULT_UNDEFINED : 0,
            RESULT_UNKNOWN : 1,
            RESULT_OK : 2,
            RESULT_PARTIAL : 3,
            RESULT_CANCELED : 4,
            RESULT_ERROR : 5
        },
        url : url,               //The web server url
        ws : null               //The web socket to the server
    };
    
    window.console.log("Creating a new ws client for the server: " + url);
    
    //Set the url into the server input
    url_input.val(url);

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
     * This function is called if one needs to connect or re-connect to the translation server.
     */
    function connect_to_server() {
        window.console.log("Disable the controls before connecting to a new server");
        disable_interface_fn();

        try {
            window.console.log("Checking that the web socket connection is available");
            if (window.hasOwnProperty("WebSocket")) {
                window.console.log("Close the web socket connection if there is one");
                if ((client.ws !== null) && ((client.ws.readyState === window.WebSocket.CONNECTING) ||
                                            (client.ws.readyState === window.WebSocket.OPEN))) {
                    logger_mdl.info("Closing the previously opened connection");
                    update_conn_status(window.WebSocket.CLOSING);
                    client.ws.close();
                }

                logger_mdl.info("Opening a new web socket to the server: " + client.url);
                client.ws = new window.WebSocket(client.url);

                update_conn_status(window.WebSocket.CONNECTING);

                window.console.log("Set up the socket handler functions");
                
                //Set the on connection open handler
                client.ws.onopen = function () {
                    logger_mdl.success("The connection to '" + client.url + "' is open");
                    update_conn_status(window.WebSocket.OPEN);

                    //Call the on open connection handler
                    if (on_open_fn) {
                        on_open_fn();
                    } else {
                        logger_mdl.warning("The on-open handler for server " + url + " is not set!");
                    }

                    //Enable the interface controls
                    enable_interface_fn(true);
                };
                
                //Set the on message handler
                client.ws.onmessage = function (evt) {
                    //log the json data
                    window.console.log("Received message:" + evt.data);

                    window.console.log("Parsing to JSON");
                    var resp_obj = JSON.parse(evt.data);

                    //Check if the message type is detectable
                    if (resp_obj.hasOwnProperty('msg_type')) {
                        //Call the on message handler with the json object
                        if (on_message_fn) {
                            on_message_fn(resp_obj);
                        } else {
                            logger_mdl.warning("The on-message handler for server " + url + " is not set!");
                        }
                    } else {
                        logger_mdl.danger("Malformed server message: " + evt.data);
                    }
                };
                
                //Set the on connection close handler
                client.ws.onclose = function () {
                    update_conn_status(window.WebSocket.CLOSED);
                    
                    //Call the on closed connection handler
                    if (on_close_fn) {
                        on_close_fn();
                    } else {
                        logger_mdl.warning("The on-close handler for server " + url + " is not set!");
                    }

                    //Enable the interface controls
                    enable_interface_fn(false);
                };
            } else {
                //Disable the web page
                logger_mdl.danger("The WebSockets are not supported by your browser!");
            }
        } catch (err) {
            enable_interface_fn(false);
        }
    }
        
    /**
     * This function is called if the server URL change event is fired,
     * so we need to check if we need to (re-)connect.
     */
    function on_url_change() {
        var url;

        //Get the current value and trim it
        url = url_input.val().trim();
        //Put the trimmed value back into the input
        url_input.val(url);

        window.console.log("The new server url is: " + url);

        if ((client.ws === null) || (client.ws.readyState !== window.WebSocket.OPEN) || (client.url !== url)) {

            //A new server means new translation
            needs_new_trans_fn();

            window.console.log("Storing the new server url value");
            client.url = url;

            if (client.url !== "") {
                window.console.log("Connecting to the new server url");
                connect_to_server();
            }
        } else {
            window.console.log("The server url has not changed");
        }
    }

    //Set the server change handlers
    url_input.change(on_url_change);
    url_input.focus(on_url_change);
    
    //Set the server connection function
    client.connect_fn = connect_to_server;

    return client;
}