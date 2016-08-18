/**
 * This function is to be called to create the Web Socket server client
 * @param name {String} the name of the server we are connecting to
 * @param url {String} the initial server url to begin with
 * @patam url_input {Input} the input object storing the server url
 * @param needs_new_trans {Function} the function that allows to notify that a new translation is needed
 * @param ui_off {Function} disable the user interface
 * @param ui_on {Function} enable the user interface
 * @param on_open_cb {Function} the function to be called when the socket is connected
 * @param on_message_cb {Function} the function to be called when the a message is received
 * @param on_close_cb {Function} the function to be called when the socket is closed
 * @param conn_status {Function} the function to set the connection status
 * @return {Object} the Web Socket server client.
 */
function create_ws_client(name, url, url_input, needs_new_trans, ui_off,
                          ui_on, on_open_cb, on_message_cb, on_close_cb,
                          conn_status) {
    "use strict";
    
    //Declare the function that is to be called once the server url has changed
    var on_url_change = function () {
        var new_url;

        //Get the current value and trim it
        new_url = this.url_input.value.trim();
        //Put the trimmed value back into the input
        this.url_input.value = new_url;

        window.console.log("The new " + this.name + " server url is: " + new_url);

        if ((this.ws.readyState !== window.WebSocket.OPEN) || (this.url !== new_url)) {
            //A new server means new translation
            this.needs_new_trans();

            window.console.log("Storing the new " + this.name + " server url value");
            this.url = new_url;

            if (this.url !== "") {
                window.console.log("Connecting to the new " + this.name + " server url");
                this.connect_to_server();
            }
        } else {
            window.console.log("The " + this.name + " server url has not changed");
        }
    },
        connect_to_server = function () {
            window.console.log("Disable the controls before connecting to a new server");
            this.ui_off();

            try {
                window.console.log("Checking that the web socket connection is available");
                if (window.hasOwnProperty("WebSocket")) {
                    window.console.log("Close the web socket connection if there is one");
                    if ((this.ws !== null) && ((this.ws.readyState === window.WebSocket.CONNECTING) ||
                                                      (this.ws.readyState === window.WebSocket.OPEN))) {
                        this.info("Closing the previously opened connection");
                        this.conn_status(window.WebSocket.CLOSING);
                        this.ws.close();
                    }

                    this.info("Opening a new web socket to the server: " + this.url);
                    this.ws = new window.WebSocket(this.url);

                    this.conn_status(window.WebSocket.CONNECTING);

                    window.console.log("Set up the socket handler functions");
                    this.ws.onopen = this.on_open;
                    this.ws.onmessage = this.on_message;
                    this.ws.onclose = this.on_close;
                } else {
                    //Disable the web page
                    this.danger("The WebSockets are not supported by your browser!");
                }
            } catch (err) {
                this.ui_on(false);
            }
        };

    //Return the object
    return {
        name : name,
        url : url,
        ws : null,
        url_input : url_input,
        on_url_change : on_url_change,
        needs_new_trans : needs_new_trans,
        connect_to_server : connect_to_server,
        ui_off : ui_off,
        ui_on : ui_on,
        on_open_cb : on_open_cb,
        on_message_cb : on_message_cb,
        on_close_cb : on_close_cb,
        on_open : function () {
            this.success("The connection to '" + this.url + "' is open");
            this.conn_status(window.WebSocket.OPEN);

            //Call the client specified function
            on_open_cb();
            
            //Enable the interface controls
            this.ui_on(true);
        },
        on_message : function () {
            
        },
        on_close : function () {
            
        },
        conn_status : conn_status
    };
}