/* 
 * File:   processor_parameters.hpp
 * Author: Dr. Ivan S. Zapreev
 *
 * Visit my Linked-in profile:
 *      <https://nl.linkedin.com/in/zapreevis>
 * Visit my GitHub:
 *      <https://github.com/ivan-zapreev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.#
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created on July 25, 2016, 11:41 AM
 */

#ifndef PROCESSOR_PARAMETERS_HPP
#define PROCESSOR_PARAMETERS_HPP

#include <map>
#include <string>

#include "common/messaging/language_registry.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                //Forward declaration
                struct language_config_struct;

                //Typedef the structure
                typedef language_config_struct language_config;

                /**
                 * The structure for storing the language config parameters
                 */
                struct language_config_struct {
                    //Stores working dir tempalate parameter name
                    static const string WORK_DIR_TEMPL_PARAM_NAME;
                    //Stores processor job unique id template parameter name
                    static const string JOB_UID_TEMPL_PARAM_NAME;
                    //Stores language template parameter name
                    static const string LANGUAGE_TEMPL_PARAM_NAME;

                    /**
                     * The basic constructor for the structure that allows
                     * to set the reference to the work directory
                     * @param work_dir  the reference to the work directory name
                     */
                    language_config_struct(const string & work_dir)
                    : m_work_dir(work_dir), m_call_templ("") {
                    }

                    /**
                     * Allows to check if the language configuration is set.
                     * I.e. that there is call template set.
                     * @return true if the call template is set, otherwise false.
                     */
                    inline bool is_defined() const {
                        return !m_call_templ.empty();
                    }

                    /**
                     * Allows to get the pre-processor call for the given job id and language.
                     * Note: This is not very optimal to search each time, can be optimized by
                     * not searching for at least one of the template parameters
                     * @param file_name the text storing file name
                     * @param lang the source language, will be lowercased
                     * @return a ready to call string
                     */
                    inline string get_call_string(const string file_name, string lang) const {
                        if (!m_call_templ.empty()) {
                            string result = m_call_templ;
                            replace(result, WORK_DIR_TEMPL_PARAM_NAME, m_work_dir);
                            replace(result, JOB_UID_TEMPL_PARAM_NAME, file_name);
                            replace(result, LANGUAGE_TEMPL_PARAM_NAME, lang);
                            return result;
                        } else {
                            return "";
                        }
                    }

                    /**
                     * Allows to set the call template, throws in case the
                     * template parameters are not found!
                     * @param call_templ the call template
                     */
                    inline void set_call_template(string & call_templ) {
                        //Store the template
                        m_call_templ = call_templ;

                        //Check the presence of the parameters
                        check_parameter(WORK_DIR_TEMPL_PARAM_NAME);
                        check_parameter(JOB_UID_TEMPL_PARAM_NAME);
                        check_parameter(LANGUAGE_TEMPL_PARAM_NAME);
                    }

                    /**
                     * Allows to get the work directory name
                     * @return the work directory name
                     */
                    inline const string & get_work_dir() const {
                        return m_work_dir;
                    }

                private:
                    //Stores the reference to the work directory name string
                    const string & m_work_dir;
                    //Stores the script call template
                    string m_call_templ;

                    /**
                     * Allows to check the presence of the template parameter
                     * place holder in the script call string.
                     * @param param parameter name
                     * @throws uva_exception in case the parameter is not found
                     */
                    inline void check_parameter(const string & param) const {
                        size_t pos = m_call_templ.find(param);
                        ASSERT_CONDITION_THROW(pos == std::string::npos,
                                string("The call template: '") + m_call_templ +
                                string("' does not contain template parameter '") +
                                param + string("'"));
                    }

                    //Add the output stream class as a friend
                    friend std::ostream& operator<<(std::ostream&, const language_config &);
                };

                //Typedef the pointer to the language config
                typedef language_config* language_config_ptr;

                //Typedef the language id to configuration map
                typedef map<language_uid, language_config_ptr> lang_to_conf_map;

                /**
                 * This is the storage for processor parameters:
                 * Responsibilities:
                 *      Store the run-time parameters of the processor application
                 */
                struct processor_parameters_struct {
                    //Stores the configuration section name
                    static const string CONFIG_SECTION_NAME;
                    //Stores the server port parameter name
                    static const string SERVER_PORT_PARAM_NAME;

                    //Stores the number of request threads parameter name
                    static const string NUM_THREADS_PARAM_NAME;

                    //Stores the work directory parameter name
                    static const string WORK_DIR_PARAM_NAME;

                    //Stores the pre-processor script call template parameter name
                    static const string PRE_CALL_TEMPL_PARAM_NAME;
                    //Stores the post-processor script call template parameter name
                    static const string POST_CALL_TEMPL_PARAM_NAME;

                    //The port to listen to
                    uint16_t m_server_port;

                    //The number of the threads handling the requests
                    size_t m_num_threads;

                    //The work directory for storing input and output files
                    string m_work_dir;

                    //The pre-processor script call template, if
                    //empty then there is no script given
                    language_config m_pre_script_config;

                    //The post-processor script call template, if
                    //empty then there is no script given
                    language_config m_post_script_config;

                    /**
                     * The basic constructor
                     */
                    processor_parameters_struct()
                    : m_pre_script_config(m_work_dir), m_post_script_config(m_work_dir) {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~processor_parameters_struct() {
                    }

                    /**
                     * Allows set the pre and post processor scriptcall templates.
                     * @param pre_call_templ the pre-processor script call template,
                     *                        if empty then ignored: no pre-processor,
                     *                        is trimmed.
                     * @param post_call_templ the post-processor script call template,
                     *                        if empty then ignored: no post-processor,
                     *                        is trimmed.
                     */
                    inline void set_processors(string pre_call_templ, string post_call_templ) {
                        //Trim the string
                        trim(pre_call_templ);
                        trim(post_call_templ);

                        LOG_DEBUG << "Registering: pre-processor script call: '" << pre_call_templ
                                << "', post-processor script call: '" << post_call_templ << "'" << END_LOG;

                        //Register the pre script if not empty
                        if (!pre_call_templ.empty()) {
                            m_pre_script_config.set_call_template(pre_call_templ);
                        }
                        //Register the post script if not empty
                        if (!post_call_templ.empty()) {
                            m_post_script_config.set_call_template(post_call_templ);
                        }
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    inline void finalize() {
                        ASSERT_CONDITION_THROW((!m_pre_script_config.is_defined() && !m_post_script_config.is_defined()),
                                "Neither the pre-processor nor the post-processor are configured!");

                        ASSERT_CONDITION_THROW((m_num_threads == 0),
                                string("The number of request threads: ") +
                                to_string(m_num_threads) +
                                string(" must be larger than zero! "));
                    }
                };

                //Typedef the structure
                typedef processor_parameters_struct processor_parameters;

                /**
                 * Allows to output the config object to the stream
                 * @param stream the stream to output into
                 * @param config the config object
                 * @return the stream that we output into
                 */
                std::ostream& operator<<(std::ostream& stream, const language_config & config);

                /**
                 * Allows to output the parameters object to the stream
                 * @param stream the stream to output into
                 * @param params the parameters object
                 * @return the stream that we output into
                 */
                static inline std::ostream& operator<<(std::ostream& stream, const processor_parameters & params) {
                    //Dump the main server config
                    return stream << "Processor parameters: {server_port = " << params.m_server_port
                            << ", num_threads = " << params.m_num_threads
                            << ", work_dir = " << params.m_work_dir
                            << ", pre_script_conf = " << params.m_pre_script_config
                            << ", post_script_conf = " << params.m_post_script_config << "}";
                }
            }
        }
    }
}


#endif /* PROCESSOR_PARAMETERS_HPP */

