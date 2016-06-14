var settings = {};

function initialize_translation() {
    settings.is_translating=false;
};

function do_translate() {
    var progressImage = document.getElementById("progress");
    if( settings.is_translating ) {
        progressImage.src = "globe32.png";
        settings.is_translating = false;
    } else {
        progressImage.src = "globe32.gif";
        settings.is_translating = true;
    }
};