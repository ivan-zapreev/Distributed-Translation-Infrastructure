/**
 * Allows to create a new web socket server client
 * @param log_panel {Dom object} the log panel to put the messages into
 * @param get_date_fn {Function} the function to get the date string
 * @param escape_html_fn {Function} the function to escape html
 * @return the web socket client module
 */
function create_logger(log_panel, lp_dang, lp_warn, lp_info, lp_succ, get_date_fn, escape_html_fn) {
    "use strict";

    /**
     * ALlows to change the badge value
     * @param {Jquery Object} badge the badge object to work with
     * @param {Boolean} is_set if true then the value will be set, otherwise added
     * @param {Number} factor the factor to add/set to the badge value
     */
    function change_badge_value(badge, is_set, factor) {
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
     * Allows to add a message of a given type to log
     * @param {Json Object} the badge object to get the data set into
     * @param {String} type the log entry type
     * @param {String} message the message to be placed
     * @param {Boolean} is_alert true if an alert box is to be show, otherwise false, optional, default is false
     */
    function add_log_message(badge, type, message, is_alert) {
        is_alert = is_alert || false;

        var value;

        //Increment the number of messages of the given sort
        change_badge_value(badge, false, +1);

        log_panel.append(
            "<div class='alert alert-" + type + " fade in'>" + get_date_fn() +
                " | <strong>" + type + ":</strong> " + escape_html_fn(message) + "</div>"
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
        is_alert = is_alert || false;

        add_log_message(lp_dang, "danger", message, is_alert);

        window.console.error(message);
    }

    /**
     * This function is called in case a warning message is to be logged
     * @param {String} message the message to visualize
     */
    function warning(message, is_alert) {
        is_alert = is_alert || false;

        add_log_message(lp_warn, "warning", message, is_alert);

        window.console.warn(message);
    }

    /**
     * This function is called in case an info message is to be logged
     * @param {String} message the message to visualize
     */
    function info(message, is_alert) {
        is_alert = is_alert || false;

        add_log_message(lp_info, "info", message, is_alert);

        window.console.log(message);
    }

    /**
     * This function is called in case a success message is to be logged.
     * @param {String} message the message to visualize
     */
    function success(message) {
        add_log_message(lp_succ, "success", message);

        window.console.info(message);
    }

    /**
     * Allows to clear the log panel and re-set the badges
     */
    function clear_log() {
        //Clear the log panel
        log_panel.html("");
        //Re-set the badges
        change_badge_value(lp_dang, true, 0);
        change_badge_value(lp_warn, true, 0);
        change_badge_value(lp_info, true, 0);
        change_badge_value(lp_succ, true, 0);
    }
    
    //Create the logger module
    var logger = {
        "danger" : danger,
        "warning" : warning,
        "info" : info,
        "success" : success,
        "clear_log" : clear_log
    };
    
    return logger;
}