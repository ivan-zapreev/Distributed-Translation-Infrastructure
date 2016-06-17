var client_data = {
    "server_url" : "ws://localhost:9002", //The web server url
    "ws" : null,                          //The web socket to the server
    "active_translations" : 0             //The number of active translations
};

//Declare the prefix of the translation job response message
var TRANS_JOB_RESPONSE_PREFIX = "TRAN_JOB_RESP";
//Declare the prefix of the supported languages response message
var SUPP_LANG_RESPONSE_PREFIX = "SUPP_LANG_RESP";

function update_conn_status(ws_status) {
    "use strict";
    var status_span = document.getElementById("conn_status");
    switch (ws_status) {
    case window.WebSocket.CONNECTING:
        status_span.innerHTML = "Connecting ...";
        break;
    case window.WebSocket.OPEN:
        status_span.innerHTML = "Connected";
        break;
    case window.WebSocket.CLOSING:
        status_span.innerHTML = "Disconnecting ...";
        break;
    case window.WebSocket.CLOSED:
        status_span.innerHTML = "Disconnected";
        break;
    default:
        status_span.innerHTML = "Puzzled :)";
        break;
    }
}

function update_trans_status() {
    "use strict";
    
    var progressImage = document.getElementById("progress");
    if (client_data.active_translations === 0) {
        progressImage.src = "globe32.png";
    } else {
        progressImage.src = "globe32.gif";
    }
}

function do_translate() {
    "use strict";
    
    window.console.log("Increment the number of active translations");
    client_data.active_translations += 1;

    window.console.log("Update the translation status");
    update_trans_status();
}

function disable_controls() {
    "use strict";
    document.getElementById("server_url").disabled = true;
    document.getElementById("trans_btn").disabled = true;
    document.getElementById("trans_cb").disabled = true;
    document.getElementById("from_text").disabled = true;
    document.getElementById("from_lang_sel").disabled = true;
    document.getElementById("to_lang_sel").disabled = true;
}

function enable_controls() {
    "use strict";
    document.getElementById("server_url").disabled = false;
    document.getElementById("trans_btn").disabled = false;
    document.getElementById("trans_cb").disabled = false;
    document.getElementById("from_text").disabled = false;
    document.getElementById("from_lang_sel").disabled = false;
    document.getElementById("to_lang_sel").disabled = false;
}

function error(message) {
    "use strict";
    
    window.alert("ERROR: " + message);
}

function on_open() {
    "use strict";
    
    window.console.log("Connection is open");
    update_conn_status(window.WebSocket.OPEN);

    //Sent the request for the supported languages
    client_data.ws.send("SUPP_LANG_REQ");

    window.console.log("Enable the controls after connecting to a new server");
    enable_controls();
}

function set_translation(trans_response) {
    "use strict";
    
    window.alert("Implement parsing the translation response!");
}

function on_source_lang_select() {
    "use strict";
    var i, to_select, from_select, source_lang, targets;
    
    window.console.log("The source language is selected");
 
    to_select = document.getElementById("to_lang_sel");
    to_select.innerHTML = "<option value=\"\">Please select</option>";
    
    from_select = document.getElementById("from_lang_sel");
    source_lang = from_select.options[from_select.selectedIndex].value;

    if (source_lang !== "") {
        targets = client_data.language_mapping[source_lang];
        window.console.log(source_lang + " -> [ " + targets + " ]");
        for (i = 0; i < targets.length; i += 1) {
            to_select.innerHTML += "<option value=\"" + targets[i] + "\">" + targets[i] + "</option>";
        }
    } else {
        to_select.innerHTML = "";
    }
}

function set_supported_languages(languages) {
    "use strict";
    var source_lang, to_select, from_select;
    
    window.console.log("Parsing the supported languages response to JSON: " + languages);
    client_data.language_mapping = JSON.parse(languages);

    to_select = document.getElementById("to_lang_sel");
    to_select.innerHTML = "";

    window.console.log("Update the from and to select boxes");
    from_select = document.getElementById("from_lang_sel");
    from_select.innerHTML = "<option value=\"\">Please select</option>";

    window.console.log("Add the available source languages");
    for (source_lang in client_data.language_mapping) {
        if (client_data.language_mapping.hasOwnProperty(source_lang)) {
            from_select.innerHTML += "<option value=\"" + source_lang + "\">" + source_lang + "</option>";
        }
    }
}

function on_message(evt) {
    "use strict";
    
    window.console.log("Message is received: " + evt.data);

    //Check of the message type
    if (evt.data.startsWith(TRANS_JOB_RESPONSE_PREFIX)) {
        set_translation(evt.data);
    } else {
        if (evt.data.startsWith(SUPP_LANG_RESPONSE_PREFIX)) {
            set_supported_languages(evt.data.slice(SUPP_LANG_RESPONSE_PREFIX.length));
        } else {
            error("An unknown server message: " + evt.data);
        }
    }
}

function on_close() {
    "use strict";
    
    window.console.log("Connection is closed");
    update_conn_status(window.WebSocket.CLOSED);

    window.console.log("Re-set the counter for the number of running translations to zero");
    client_data.active_translations = 0;
    window.console.log("Update the translation status");
    update_trans_status();

    window.console.log("The connection to: '" + client_data.server_url + "'has failed!");

    window.console.log("Enable the controls after disconnecting from a server");
    enable_controls();
}

function connect_to_server() {
    "use strict";
    
    window.console.log("Disable the controls before connecting to a new server");
    disable_controls();

    window.console.log("Checking that the web socket connection is available");
    if (window.hasOwnProperty("WebSocket")) {
        window.console.log("Close the web socket connection if there is one");
        if (client_data.ws !== null) {
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

function on_server_change() {
    "use strict";
    var server_url_inpt, new_server_url;
    
    server_url_inpt = document.getElementById("server_url");
    new_server_url = server_url_inpt.value;
    new_server_url = new_server_url.trim();
    server_url_inpt.value = new_server_url;
    window.console.log("The new server url is: " + new_server_url);

    if ((client_data.ws.readyState !== window.WebSocket.OPEN) || (client_data.server_url !== new_server_url)) {
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

function initialize_translation() {
    "use strict";
    
    window.console.log("Initializing the client, url: " + client_data.server_url);

    //Set up the page
    document.getElementById("server_url").value = client_data.server_url;

    window.console.log("Open an initial connection to the server");

    //Connect to the server - open websocket connection
    connect_to_server();
}
