/* 
 * File:   server_parameters.hpp
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
 * Created on February 4, 2016, 11:51 AM
 */

#ifndef SERVER_PARAMETERS_HPP
#define SERVER_PARAMETERS_HPP

#include <string>
#include <ostream>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "decoder/de_parameters.hpp"
#include "lm/lm_parameters.hpp"
#include "rm/rm_parameters.hpp"
#include "tm/tm_parameters.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * This structure stores the translation server parameters
                 */
                typedef struct {
                    //The source language name
                    string m_source_lang;
                    //The target language name
                    string m_target_lang;

                    //The port to listen to
                    uint16_t m_server_port;

                    //The number of the translation threads to run
                    size_t m_num_threads;

                    //Stores the translation model parameters
                    tm_parameters m_tm_params;

                    //Stores the reordering model parameters
                    rm_parameters m_rm_params;

                    //Stores the language model parameters
                    lm_parameters m_lm_params;

                    //Stores the decoder parameters
                    de_parameters m_de_params;

                    /**
                     * Allows to verify the parameters to be correct.
                     */
                    void verify() {
                        ASSERT_CONDITION_THROW((m_num_threads == 0),
                                string("The number of decoding threads: ") +
                                to_string(m_num_threads) +
                                string(" must be larger than zero! "));
                    }
                } server_parameters;

                /**
                 * Allows to output the parameters object to the stream
                 * @param stream the stream to output into
                 * @param params the parameters object
                 * @return the stream that we output into
                 */
                static inline std::ostream& operator<<(std::ostream& stream, const server_parameters & params) {
                    return stream << "Server parameters:\nMain [ source_lang = " << params.m_source_lang
                            << ", target_lang = " << params.m_target_lang
                            << ", server_port = " << params.m_server_port
                            << ", num_threads = " << params.m_num_threads
                            << "]\n" << params.m_de_params
                            << "\n" << params.m_lm_params
                            << "\n" << params.m_tm_params
                            << "\n" << params.m_rm_params;
                }
            }
        }
    }
}
#endif /* SERVER_PARAMETERS_HPP */

