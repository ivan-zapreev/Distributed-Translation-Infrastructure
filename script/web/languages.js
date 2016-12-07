/*
 *  File:   languages.js
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

//Declare the "Please select" string for the srouce/target language select boxes
var PLEASE_SELECT_STRING = "...";
//The detect option string for automatic language detection 
var AUTO_DETECT_STRING = "--detect--";
//The auto string to be sent to the pre-processor
var AUTO_DETECT_OPTION = "auto";

/**
 * Allows to create a new languages module
 * @param logger_mdl {Object} the logger module
 * @param ...
 * @return the languages module
 */
function create_languages(logger_mdl, from_lang_sel, to_lang_sel, needs_new_trans_fn) {
    "use strict";

    //Define the module
    var module = {
        language_mapping : {},   //Will store the supported language pairs
        set_supp_langs_fn : null //Will store the pointer to the function for setting the supported languages
    };
 
    /**
     * Allows to get the selected element value from the select element with the given id
     * @param {Object} the select element to get the selected value from
     * @param {String} the value of the selected element
     */
    function get_selected_lang(select) {
        var selected_lang;

        selected_lang = select.find('option:selected').val();
        
        //The value can be undefined if, e.g., the
        //supported languages list is yet empty.
        if (typeof selected_lang === 'undefined') {
            selected_lang = "";
        }
        
        window.console.log("The selected '" + select + "' language is: " + selected_lang);

        return selected_lang.trim();
    }

    /**
     * Allows to check if a language is selected, i.e. is also not auto
     * @param {Object} the select element to get the selected value from
     * @param {Boolean} true if a language is selected, otherwise false
     */
    function is_language_selected(select) {
        //Get the selected language
        var selected = get_selected_lang(select);

        //The selected value must not be empty
        return (selected !== "") && (selected !== AUTO_DETECT_OPTION);
    }

    /**
     * Allows to check if the source language is selected
     * @param {Boolean} true if a language is selected, otherwise false
     */
    function is_source_lang_selected() {
        return is_language_selected(from_lang_sel);
    }

    /**
     * Allows to check if the target language is selected
     * @param {Boolean} true if a language is selected, otherwise false
     */
    function is_target_lang_selected() {
        return is_language_selected(to_lang_sel);
    }

    /**
     * Allows to get the selected language source value
     * @return {string} the value of the selected source language
     */
    function get_selected_source_lang() {
        return get_selected_lang(from_lang_sel);
    }

    /**
     * Allows to get the selected language target value
     * @return {string} the value of the selected target language
     */
    function get_selected_target_lang() {
        return get_selected_lang(to_lang_sel);
    }

    /**
     * This function returns the option HTML tag for the select
     * @param {String} value the value for the option
     * @param {String} name the name for the option
     * @return {String} the complete <option> tag
     */
    function get_select_option(value, name) {
        return "<option value=\"" + value + "\">" + name + "</option>";
    }

    /**
     * This function allows to set a list of target languages into the selectin box
     * @param Array<String> the list of target languages to be set into the selection box
     */
    function set_target_languages(targets) {
        var i;
        
        //Re-set the target select to no options
        to_lang_sel.html("");
        
        //Check if the list of target languages is present
        if (targets !== undefined) {
            //Add "Please select" in case there is more than one target option available
            if (targets.length > 1) {
                to_lang_sel.html(get_select_option("", PLEASE_SELECT_STRING));
            }
            //Add the target languages to the selection box
            for (i = 0; i < targets.length; i += 1) {
                to_lang_sel.html(to_lang_sel.html() + get_select_option(targets[i], targets[i]));
            }
        } else {
            //If the list of target languages is empty then just fill it with something.
            to_lang_sel.html(get_select_option("", PLEASE_SELECT_STRING));
        }
    }
    
    /**
     * This function is called in case once thanges the selection of the source language for the translation.
     * This function then loads the appropriate list of the supported target languages.
     */
    function on_source_lang_select() {
        var source_lang, targets;

        //A new source language means new translation
        needs_new_trans_fn();

        window.console.log("The source language is selected");
        source_lang = get_selected_source_lang();

        if (source_lang !== "") {
            window.console.log("The source language is not empty!");
            //Obtain the list of targets corresponding to the source language
            targets = module.language_mapping[source_lang];
            window.console.log(source_lang + " -> [ " + targets + " ]");
            //Set the targets into the selection box
            set_target_languages(targets);
        }
    }
    
    //Set the on source language selection handler
    from_lang_sel.change(on_source_lang_select);

    /**
     * This function allows to set the supported languages from the supported languages response from the server
     * @param {Object} supp_lang_resp the supported languages response message
     */
    function set_supported_languages(supp_lang_resp) {
        var source_lang, num_sources, all_targets, all_targets_uni;

        logger_mdl.success("Received a supported languages response from the server!");

        //Store the supported languages
        module.language_mapping = supp_lang_resp.langs;

        //Filter the source languages without the targets
        //It can happen that for the soure language there is no targets, then ignore it.
        //We do it here and not on the server as here it is easier to check and on the
        //server it will require extra computation effort and will complicate the code.
        window.console.log("Filter the empty source languages");
        for (source_lang in module.language_mapping) {
            if (module.language_mapping.hasOwnProperty(source_lang)) {
                if (module.language_mapping[source_lang].length === 0) {
                    delete module.language_mapping[source_lang];
                }
            }
        }

        window.console.log("Clear the to-select as we are now re-loading the source languages");
        to_lang_sel.html(get_select_option("", PLEASE_SELECT_STRING));

        //Do not add "Please select" in case there is just one source option possible"
        num_sources = Object.keys(module.language_mapping).length;
        window.console.log("The number of source languages is: " + num_sources);

        //Only add the 'auto detection' in case there is multiple source languages
        if (num_sources > 1) {
            window.console.log("Multiple source languages: Adding 'auto detection'");
            from_lang_sel.html(get_select_option(AUTO_DETECT_OPTION, AUTO_DETECT_STRING));
        } else {
            //Re-set the source select to no options
            from_lang_sel.html("");
        }

        window.console.log("Add the available source languages");
        for (source_lang in module.language_mapping) {
            if (module.language_mapping.hasOwnProperty(source_lang)) {
                from_lang_sel.html(from_lang_sel.html() + get_select_option(source_lang, source_lang));
            }
        }

        //Check if the auto detection might be needed, i.e. there is more than one source language
        if (num_sources > 1) {
            //Add the option for the language we want to translate the auto into
            all_targets = [];
            for (source_lang in module.language_mapping) {
                if (module.language_mapping.hasOwnProperty(source_lang)) {
                    all_targets.push.apply(all_targets, module.language_mapping[source_lang]);
                }
            }
            //Filter out the duplicates
            all_targets_uni = [];
            window.$.each(all_targets, function (i, el) {
                if (window.$.inArray(el, all_targets_uni) === -1) {
                    all_targets_uni.push(el);
                }
            });
            //Set the all targets for the auto detection option
            module.language_mapping[AUTO_DETECT_OPTION] = all_targets_uni;
        }
        
        //Set the appropriate target languages
        on_source_lang_select();
    }
    
    /**
     * Checks if the source language is set to auto detect, if it is then
     * remembers the selected target language. Checks if the source language
     * is supported by the translator, if not then returns false. If it is then
     * checks if the target language for the given source languahe is supported
     * by the server if yes return true else false.
     * @param source_lang {String} the source language
     * @return true if the source/target language pair is valid, otherwise false
     */
    function set_detected_source_lang(source_lang) {
        window.console.log("Trying to set the 'auto-detected' source language: " + source_lang);
        if (module.language_mapping.hasOwnProperty(source_lang)) {
            var target_lang, targets;
            window.console.log("The source language: " + source_lang + " is supported.");
            target_lang = get_selected_target_lang();
            window.console.log("The currently selected target language: " + target_lang);
            targets = module.language_mapping[source_lang];
            //Check if the target language is supported for the given source one
            if (window.$.inArray(target_lang, targets) >= 0) {
                //The target language is present
                from_lang_sel.val(source_lang);
                //Updat the targets list and re-select the target
                set_target_languages(targets);
                //Set the current target language as selected again
                to_lang_sel.val(target_lang);
                //The language was successfully set
                return true;
            } else {
                //The target language is not present
                logger_mdl.danger("The translation server does not support: " + source_lang +
                                  " -> " + target_lang + " language pair.", true);
                return false;
            }
        } else {
            //The soure language is not present
            logger_mdl.danger("The translation server does not support: " + source_lang, true);
            return false;
        }
    }
    
    //Set the function for setting the language pairs
    module.set_supp_langs_fn = set_supported_languages;
    //Set the language selected checking functions
    module.is_source_lang_sel_fn = is_source_lang_selected;
    module.is_target_lang_sel_fn = is_target_lang_selected;
    //Set the selected language retrieval functions
    module.get_sel_source_lang_fn = get_selected_source_lang;
    module.get_sel_target_lang_fn = get_selected_target_lang;
    //Set the function for setting the source language
    module.set_detected_source_lang_fn = set_detected_source_lang;
    
    return module;
}