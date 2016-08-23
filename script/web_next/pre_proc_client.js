/**
 * Allows to create a new pre-processor server client
 * @param module {Object} the processor client module
 * @param trans_serv_mdl {Object} the translation module
 * @return the pre-processor server client module
 */
function create_pre_proc_client(module, trans_serv_mdl) {
    "use strict";

    /**
     * This function is called when the translation process needs to begin
     * @patam source_md5 {String} the source text md5 sum to be used as a job token
     * @param source_text {String} the source text to work with
     */
    function process(source_md5, source_text) {
        window.console.log("Starting pre-processing text: " + source_text);

        //Get the source text language
        var source_lang = module.language_mdl.get_sel_source_lang_fn();
        
        //Check if the processor is connected or not
        if (module.is_connected_fn()) {
            //If connected then get the source language and start sending the data
            window.console.log("The pre-processing module is connected!");
            
            //Send the processor request in chunks
            module.send_requests_fn(source_md5, source_text, source_lang);
        } else {
            window.console.log("The pre-processing module is disconnected!");
            //Check if the language is selected, i.e. also not autodetect.
            if (module.language_mdl.is_source_lang_sel_fn()) {
                window.console.log("The source language is selected to: " + source_lang);
                trans_serv_mdl.process_fn(source_text);
            } else {
                module.process_stop_fn(true, "The pre-processor is not connected, the language detection is not possible!");
            }
        }
    }
    
    //Store the pre-processing function pointer
    module.process_fn = process;
    
    return module;
}
