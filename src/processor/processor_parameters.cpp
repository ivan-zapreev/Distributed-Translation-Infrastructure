/* 
 * File:   processor_parameters.cpp
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
 * Created on July 25, 2016, 11:42 AM
 */

#include "processor/processor_parameters.hpp"

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {

                const string language_config::PROC_JOB_ID_TEMPL_PARAM_NAME = "<PROC_JOB_UID>";
                const string language_config::LANGUAGE_TEMPL_PARAM_NAME = "<TARGET_LANG>";

                const string processor_parameters::CONFIG_SECTION_NAME = "Server Options";
                const string processor_parameters::SERVER_PORT_PARAM_NAME = "server_port";
                const string processor_parameters::NUM_THREADS_PARAM_NAME = "num_threads";
                const string processor_parameters::WORK_DIR_PARAM_NAME = "work_dir";
                const string processor_parameters::PRE_CALL_TEMPL_PARAM_NAME = "pre_call_templ";
                const string processor_parameters::POST_CALL_TEMPL_PARAM_NAME = "post_call_templ";
                const string processor_parameters::LANG_CONFIGS_PARAM_NAME = "land_configs";
                const string processor_parameters::LANG_CONFIGS_DELIMITER_STR = "|";

                std::ostream& operator<<(std::ostream& stream, const language_config & config) {
                    if (!config.m_lang.empty() || !config.m_call_templ.empty()) {
                        stream << "{";

                        //Output the language if it is not empty
                        if (!config.m_lang.empty()) {
                            stream << "lang = " << config.m_lang << ", ";
                        }

                        //Output the call template if it is not empty
                        if (!config.m_call_templ.empty()) {
                            if (!config.m_lang.empty()) {
                                stream << ", ";
                            }
                            stream << "call_templ = " << config.m_call_templ;
                        }

                        stream << "}";
                    } else {
                        stream << "NONE";
                    }

                    return stream;
                }

            }
        }
    }
}