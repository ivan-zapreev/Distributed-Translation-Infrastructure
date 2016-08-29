/* 
 * File:   client_parameters.hpp
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
 * Created on January 26, 2016, 12:13 PM
 */

#ifndef CLIENT_PARAMETERS_HPP
#define CLIENT_PARAMETERS_HPP

#include <regex>
#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {

                //Stores the default processor uri
                static const string DEFAULT_PROCESSOR_URI = "";

                /**
                 * This structure stores the translation client execution parameters
                 */
                struct client_parameters_struct {
                    //The source file name with the text to translate
                    string m_source_file;
                    //The language to translate from
                    string m_source_lang;
                    //The target file name to put the result into
                    string m_target_file;
                    //The language to translate into
                    string m_target_lang;

                    //The pre-processor text server URI to connect to, if empty then no need to post-process
                    string m_pre_uri;
                    //The server URI to connect to
                    string m_trans_uri;
                    //The post-processor text server URI to connect to, if empty then no need to post-process
                    string m_post_uri;

                    //The flag indicating whether the client requests the translation details from the translation server or not.
                    bool m_is_trans_info;
                    //The maximum number of source sentences to send per translation request
                    uint64_t m_max_sent;
                    //The minimum number of source sentences to send per translation request
                    uint64_t m_min_sent;
                    
                    //Stores the priority to be used for this client
                    int32_t m_priority;

                    /**
                     * The basic constructor
                     */
                    client_parameters_struct()
                    : m_uri_reg_exp("ws://.*:\\d+") {
                    }

                    /**
                     * Allows to check if pre-processing is needed
                     * @return true if pre-processing is needed
                     */
                    inline bool is_pre_process() const {
                        return !m_pre_uri.empty();
                    }

                    /**
                     * Allows to check if post-processing is needed
                     * @return true if post-processing is needed
                     */
                    inline bool is_post_process() const {
                        return !m_post_uri.empty();
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    inline void finalize() {
                        //Check the pre-processor server uri format
                        if (is_pre_process()) {
                            ASSERT_CONDITION_THROW(!regex_match(m_pre_uri, m_uri_reg_exp),
                                    string("The pre-processor uri: '") + m_pre_uri +
                                    string("' does not match the format: ws://<server>:<port>"));
                        }

                        //Check the translation server uri format
                        ASSERT_CONDITION_THROW(!regex_match(m_trans_uri, m_uri_reg_exp),
                                string("The translation server uri: '") + m_trans_uri +
                                string("' does not match the format: ws://<server>:<port>"));

                        //Check the post-processor server uri format
                        if (is_post_process()) {
                            ASSERT_CONDITION_THROW(!regex_match(m_post_uri, m_uri_reg_exp),
                                    string("The post-processor uri: '") + m_post_uri +
                                    string("' does not match the format: ws://<server>:<port>"));
                        }
                    }

                private:
                    //The regular expression for matching the server uri
                    const regex m_uri_reg_exp;
                };

                typedef client_parameters_struct client_parameters;
            }
        }
    }
}

#endif