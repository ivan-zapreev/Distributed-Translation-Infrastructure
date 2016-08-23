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
     * This function is called in case once thanges the selection of the source language for the translation.
     * This function then loads the appropriate list of the supported target languages.
     */
    function on_source_lang_select() {
        var i, source_lang, targets;

        //A new source language means new translation
        needs_new_trans_fn();

        window.console.log("The source language is selected");
        source_lang = get_selected_source_lang();

        //Re-set the target select to no options
        to_lang_sel.html("");

        if (source_lang !== "") {
            window.console.log("The source language is not empty!");
            targets = module.language_mapping[source_lang];

            //Check if the list of target languages is present
            if (targets !== undefined) {
                //Do not add "Please select" in case there is just one target option possible
                if (targets.length > 1) {
                    to_lang_sel.html(get_select_option("", PLEASE_SELECT_STRING));
                }

                window.console.log(source_lang + " -> [ " + targets + " ]");
                for (i = 0; i < targets.length; i += 1) {
                    to_lang_sel.html(to_lang_sel.html() + get_select_option(targets[i], targets[i]));
                }
            } else {
                //If the list of target languages is empty then just fill it with something.
                to_lang_sel.html(get_select_option("", PLEASE_SELECT_STRING));
            }
        }
    }
    
    //Set the on source language selection handler
    from_lang_sel.change(on_source_lang_select);

    /**
     * This function allows to set the supported languages from the supported languages response from the server
     * @param {Object} supp_lang_resp the supported languages response message
     */
    function set_supported_languages(supp_lang_resp) {
        var source_lang, num_sources, all_targets;

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

        if (num_sources > 1) {
            //Add the option for the language we want to translate the auto into
            all_targets = [];
            for (source_lang in module.language_mapping) {
                if (module.language_mapping.hasOwnProperty(source_lang)) {
                    all_targets.push.apply(all_targets, module.language_mapping[source_lang]);
                }
            }
            //Set the all targets for the auto detection option
            module.language_mapping[AUTO_DETECT_STRING] = all_targets;
        }
        
        //Set the appropriate target languages
        on_source_lang_select();
    }
    
    //Set the function for setting the language pairs
    module.set_supp_langs_fn = set_supported_languages;
    //Set the language selected checking functions
    module.is_source_lang_sel_fn = is_source_lang_selected;
    module.is_target_lang_sel_fn = is_target_lang_selected;
    //Set the selected language retrieval functions
    module.get_sel_source_lang_fn = get_selected_source_lang;
    module.get_sel_target_lang_fn = get_selected_target_lang;
    
    return module;
}