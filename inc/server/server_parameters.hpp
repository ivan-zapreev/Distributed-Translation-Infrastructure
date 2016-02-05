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

#include "decoder/decoder_parameters.hpp"
#include "lm/lm_parameters.hpp"
#include "lm/lm_config_utils.hpp"
#include "rm/rm_parameters.hpp"
#include "tm/tm_parameters.hpp"

using namespace uva::smt::translation::server::decoder;
using namespace uva::smt::translation::server::tm;
using namespace uva::smt::translation::server::rm;
using namespace uva::smt::translation::server::lm;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {

                /**
                 * This structure stores the translation server parameters
                 */
                typedef struct {
                    //Stores the translation model parameters
                    tm_parameters m_tm_params;
                    
                    //Stores the reordering model parameters
                    rm_parameters m_rm_params;
                    
                    //Stores the language model parameters
                    lm_parameters m_lm_params;
                    
                    //Stores the decoder parameters
                    decoder_parameters m_de_params;
                } server_parameters;
            }
        }
    }
}
#endif /* SERVER_PARAMETERS_HPP */

