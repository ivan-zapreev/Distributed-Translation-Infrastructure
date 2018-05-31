/*
 *  File:   client_common.js
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

/**
 * Allows to create a new common module, storing the
 * main functionality used in most other modules.
 * @param logger_mdl {Object} the logger module
 * @param lang_mdl {Object} the languages module
 * @param to_text_span {Object} the DOM span object storing the translated text
 * @param priority_select {Object} the DOM select object for choosing priority
 * @param create_ws_client_fn {Function} the function to create a web sockets client
 * @param needs_new_trans_fn {Function} the function that allows notify that the new translation is needed
 * @param enable_interface_fn {Function}  the function to enable the interface
 * @param disable_interface_fn {Function} the function to diable the interface
 * @param escape_html_fn {Function} the function to escape the html tags
 * @param process_stop_fn {Function} the function to stop the translation process
 * @return the web socket client module
 */
function create_client_common(logger_mdl, lang_mdl, to_text_span, priority_select,
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
     * Allows to set the cookie with unlimited expiration date
     * @param {name} the name to be stored
     * @param {value} the value to be stored
     */
    function set_cookie(name, value) {
        window.Cookies.set(name, value, {expires : 2147483647});
        window.Cookies.save
    }
    
    /**
     * Allows to get the cookie value
     * @param {name} the name to be stored
     * @param {def_value} the default value to be returnes in case the cookie is not set
     * @return the cookie value if it is set or otherwise the default value
     */
    function get_cookie(name, def_value) {
        var act_value = window.Cookies.get(name);
        if (window.$.isEmptyObject(act_value)){
            //Has no cookie
            return def_value;
        } else {
            //Has cookie
            return act_value;
        }
    }
    
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
    
    /**
     * Allows to get the selected translation priority value
     * @return {Integer} the priority value
     */
    function get_priority() {
        return parseInt(priority_select.find('option:selected').val(), 10);
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
        get_status_code_str_fn : get_status_code_string,
        get_priority_fn : get_priority,
        set_cookie_fn : set_cookie,
        get_cookie_fn : get_cookie
    };
    
    return module;
}