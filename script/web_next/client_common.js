
/**
 * Allows to create a new common module, storing the
 * main functionality used in most other modules.
 * @param logger_mdl {Object} the logger module
 * @param url {String} the server url to work with
 * @param ...
 * @return the web socket client module
 */
function create_client_common(logger_mdl, lang_mdl, to_text_span,
                              create_ws_client_fn, needs_new_trans_fn,
                              enable_interface_fn, disable_interface_fn,
                              escape_html_fn, process_stop_fn) {
    "use strict";
    
    var STATUS_CODE_ENUM, trans_status, module;
    
    //Initialize the status code enumeration
    STATUS_CODE_ENUM = {
        RESULT_UNDEFINED : 0,
        RESULT_UNKNOWN : 1,
        RESULT_OK : 2,
        RESULT_PARTIAL : 3,
        RESULT_CANCELED : 4,
        RESULT_ERROR : 5
    };
    //Set the initial value for the status code
    trans_status = STATUS_CODE_ENUM.RESULT_UNDEFINED;
    
    /**
     * Allows to translate the status code number into a value
     * @param {Number} status_code the status code received from the server
     * @return {String} the string representation of the status code
     */
    function get_status_code_string(status_code) {
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
     * @param {Number} the job id 
     * @param {Number} stat_code the job response status code
     * @param {String} stat_msg the status message string the status message
     */
    function visualize_status_code(job_id, stat_code, stat_msg) {
        //Keen the maximum status code as the higher the more cevere is the error
        var new_ts = window.Math.max(stat_code, trans_status);

        //Log the event
        switch (stat_code) {
        case STATUS_CODE_ENUM.RESULT_OK:
            logger_mdl.success("Job: " + job_id + " succeeded!");
            break;
        case STATUS_CODE_ENUM.RESULT_ERROR:
            logger_mdl.danger("Job: " + job_id + " failed: " + stat_msg);
            break;
        case STATUS_CODE_ENUM.RESULT_CANCELED:
            logger_mdl.warning("Job: " + job_id + " was cancelled: " + stat_msg);
            break;
        case STATUS_CODE_ENUM.RESULT_PARTIAL:
            logger_mdl.warning("Job: " + job_id + " was partially done: " + stat_msg);
            break;
        default:
            break;
        }

        //Check if we need to change the visualization
        if (trans_status !== new_ts) {
            //Keep the new status
            trans_status = new_ts;
            //Visualize
            switch (trans_status) {
            case STATUS_CODE_ENUM.RESULT_OK:
                to_text_span.css("box-shadow", "0 0 10px green");
                break;
            case STATUS_CODE_ENUM.RESULT_ERROR:
                to_text_span.css("box-shadow", "0 0 10px red");
                break;
            case STATUS_CODE_ENUM.RESULT_CANCELED:
                to_text_span.css("box-shadow", "0 0 10px orange");
                break;
            case STATUS_CODE_ENUM.RESULT_PARTIAL:
                to_text_span.css("box-shadow", "0 0 10px yellow");
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
        //Re-set the stored status and remove the visual effect
        trans_status = STATUS_CODE_ENUM.RESULT_UNDEFINED;
        to_text_span.css("box-shadow", "none");
    }
    
    //Fill in the module
    module = {
        STATUS_CODE_ENUM : STATUS_CODE_ENUM,
        logger_mdl : logger_mdl,
        lang_mdl : lang_mdl,
        to_text_span : to_text_span,
        create_ws_client_fn : create_ws_client_fn,
        needs_new_trans_fn : needs_new_trans_fn,
        enable_interface_fn : enable_interface_fn,
        disable_interface_fn : disable_interface_fn,
        escape_html_fn : escape_html_fn,
        process_stop_fn : process_stop_fn,
        visualize_sc_fn : visualize_status_code,
        remove_cs_visual_fn : remove_status_code_visual,
        get_status_code_str_fn : get_status_code_string
    };
    
    return module;
}