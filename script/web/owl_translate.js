var settings = {
    "server_url" : "ws://localhost:9002", //The web server url
    "ws" : null,                          //The web socket to the server
    "settings.active_translations" : 0,   //The number of active translations
};

function update_conn_status(ws_status) {
    var status_span = document.getElementById("conn_status");
    switch(ws_status) {
        case WebSocket.CONNECTING: 
            status_span.innerHTML = "Connecting ...";
            break;
        case WebSocket.OPEN: 
            status_span.innerHTML = "Connected";
            break;
        case WebSocket.CLOSING: 
            status_span.innerHTML = "Disconnecting ...";
            break;
        case WebSocket.CLOSED: 
            status_span.innerHTML = "Disconnected";
            break;
        default:
            status_span.innerHTML = "Puzzled :)";
            break;
    }
}

function initialize_translation() {
    console.log("Initializing the client, url: " + settings.server_url);

    //Set up the page
    document.getElementById("server_url").value = settings.server_url;

    console.log("Open an initial connection to the server");

    //Connect to the server - open websocket connection
    connect_to_server();
};

function update_trans_status() {
    var progressImage = document.getElementById("progress");
    if( settings.active_translations == 0  ) {
        progressImage.src = "globe32.png";
    } else {
        progressImage.src = "globe32.gif";
    }
}

function do_translate() {
    console.log("Increment the number of active translations");
    settings.active_translations += 1;

    console.log("Update the translation status");
    update_trans_status();
};

function disable_controls() {
    document.getElementById("server_url").disabled = true;
    document.getElementById("trans_btn").disabled = true;
    document.getElementById("trans_cb").disabled = true;
    document.getElementById("from_text").disabled = true;
    document.getElementById("from_lang_sel").disabled = true;
    document.getElementById("to_lang_sel").disabled = true;
}

function enable_controls() {
    document.getElementById("server_url").disabled = false;
    document.getElementById("trans_btn").disabled = false;
    document.getElementById("trans_cb").disabled = false;
    document.getElementById("from_text").disabled = false;
    document.getElementById("from_lang_sel").disabled = false;
    document.getElementById("to_lang_sel").disabled = false;
}

function error(message) {
    alert("ERROR: " + message);
}

function on_open() {
    console.log("Connection is open");
    update_conn_status(WebSocket.OPEN);

    //ToDo: Sent the request for the supported languages

    console.log("Enable the controls after connecting to a new server");
    enable_controls();
}

function on_message(evt)  {
    console.log("Message is received");
}

function on_close() {
    console.log("Connection is closed");
    update_conn_status(WebSocket.CLOSED);

    console.log("Re-set the counter for the number of running translations to zero");
    settings.active_translations = 0;
    console.log("Update the translation status");
    update_trans_status();

    console.log("The connection to: '" + settings.server_url + "'has failed!");

    console.log("Enable the controls after disconnecting from a server");
    enable_controls();
}

function connect_to_server() {
    console.log("Disable the controls before connecting to a new server");
    disable_controls();

    console.log("Checking that the web socket connection is available");
    if ("WebSocket" in window)
    {
        console.log("Close the web socket connection if there is one");
        if(settings.ws != null) {
             console.log("Closing the previously opened connection");
             update_conn_status(WebSocket.CLOSING);
             settings.ws.close();
        }

        console.log("Create a new web socket to: " + settings.server_url);
        settings.ws = new WebSocket(settings.server_url);

        update_conn_status(WebSocket.CONNECTING);

        console.log("Set up the socket handler functions");
        settings.ws.onopen = on_open;
        settings.ws.onmessage = on_message;
        settings.ws.onclose = on_close;
    } else {
        //Disable the web page
        error("The WebSockets are not supported by your browser!");
    }
}

function on_server_change() {
    var server_url_inpt = document.getElementById("server_url");
    var new_server_url = server_url_inpt.value;
    new_server_url = new_server_url.trim();
    server_url_inpt.value = new_server_url;
    console.log("The new server url is: " + new_server_url );

    if( (settings.ws.readyState != WebSocket.OPEN) || ( settings.server_url != new_server_url ) ) {
        console.log("Storing the new server url value" );
        settings.server_url = new_server_url;
    
        if(settings.server_url != "") {
            console.log("Connecting to the new server url" );
            connect_to_server();
        }
    } else {
        console.log("The server url has not changed" );
    }
}