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
#include "rm/rm_parameters.hpp"
#include "tm/tm_parameters.hpp"

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {

                /**
                 * This structure stores the program execution parameters
                 */
                typedef struct {
                    //The language model file name 
                    string m_language_model;
                    //The translation model file name 
                    string m_translation_model;
                    //The reordering model file name 
                    string m_reordering_model;

                    //The target language name
                    string m_target_lang;
                    //The source language name
                    string m_source_lang;

                    //The port to listen to
                    uint16_t m_server_port;

                    //The number of the translation threads to run
                    size_t m_num_threads;

                    //The distortion limit to use
                    uint32_t m_distortion_limit;
                    //The pruning threshold to be used
                    float m_pruning_threshold;
                    //The stack capacity for stack pruning
                    uint32_t m_stack_capacity;
                    //The stack expansion strategy
                    string m_expansion_strategy;
                } server_parameters;
            }
        }
    }
}
#endif /* SERVER_PARAMETERS_HPP */

