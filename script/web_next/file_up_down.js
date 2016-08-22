//Stores the number of bytes in one Mb
var NUMBER_OF_BYTES_IN_MEGABYTE = 1024 * 1024;
//Stores the maximum file limit for the file upload
var MAXIMUM_FILE_UPLOAD_SIZE_MB = 3; //Maximum 3mb in bytes

/**
 * Allows to create the file upload/download module
 * @param ...
 * @return the file upload/download module
 */
function init_file_ud(logger_mdl, from_text_area, input_file_sc, input_file_select, download_text_link, download_log_link, download_fn, get_date_fn) {
    "use strict";

    /**
     * This method should handle the finish of file reading event
     * @param {Object} evt the event that the file has been read
     */
    function on_input_file_read(evt) {
        logger_mdl.success("The file is loaded!");

        from_text_area.val(evt.target.result);
    }

    /**
     * Will be called when a text download link is clicked
     * @param {Object} the on click event 
     */
    function download_text_translation(evt) {
        var file_name, text;

        //Construct the file name and do logging
        file_name = "translation." + get_date_fn('.') + ".txt";
        window.console.log("Downloading: " + file_name);

        //Get the translations.
        text = "";
        window.$(".target_sent_tag").each(function (index) {
            text += window.$(this).text() + "\n";
        });

        window.console.log("Text: " + text);

        //Call the download function from the library
        download_fn(text, file_name, "text/html; charset=UTF-8");
    }

    /**
     * Will be called when a log download link is clicked
     * @param {Object} the on click event 
     */
    function download_log_translation(evt) {
        var file_name, text;

        //Construct the file name and do logging
        file_name = "translation." + get_date_fn('.') + ".log";
        window.console.log("Downloading: " + file_name);

        //Get the translations.
        text = "";
        window.$(".target_sent_tag").each(function (index) {
            text += window.$(this).attr("data-original-title") + "\n";
        });

        window.console.log("Text: " + text);

        //Call the download function from the library
        download_fn(text, file_name, "text/html; charset=UTF-8");
    }
    
    /**
     * The function that will be called once the upload file is selected
     * @param {Object} evt the change event
     */
    function on_upload_file_select(evt) {
        var file, reader, size_mb;

        //Get the file
        file = evt.target.files[0];
        size_mb = file.size / NUMBER_OF_BYTES_IN_MEGABYTE;
        size_mb = window.parseFloat(size_mb.toFixed(1));
        logger_mdl.info("Selected a file to translate: " + file.name + ", " + size_mb + " Mb");

        if (size_mb <= MAXIMUM_FILE_UPLOAD_SIZE_MB) {
            //Read the file into the text field
            reader = new window.FileReader();
            reader.onload = on_input_file_read;

            logger_mdl.info("Start loading the file into memory!");
            reader.readAsText(file, 'UTF-8');
        } else {
            //Clear the file input by replacing its container's html content 
            input_file_sc.html(input_file_sc.html());
            input_file_select = window.$("#input_file_select");
            input_file_select.bind('change', on_upload_file_select);
            logger_mdl.danger("The file '" + file.name + "' size is " + size_mb +
                   " Mb, the maximum we allow is " + MAXIMUM_FILE_UPLOAD_SIZE_MB +
                   " Mb!", true);
        }
    }
    
    //Check for the various File API support.
    if (window.File && window.FileReader && window.FileList && window.Blob) {
        //Add the selection function handler
        input_file_select.bind('change', on_upload_file_select);
    } else {
        logger_mdl.warning('The File APIs are not (fully) supported in this browser.');
        //Disable the file upload related elements
        input_file_select.hide();
    }
    
    //Add the link download handlers
    download_text_link.click(download_text_translation);
    download_log_link.click(download_log_translation);
}