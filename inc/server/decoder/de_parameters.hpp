/* 
 * File:   dec_parameters.hpp
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
 * Created on February 4, 2016, 11:53 AM
 */

#ifndef DEC_PARAMETERS_HPP
#define DEC_PARAMETERS_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

                    /**
                     * This structure stores the decoder parameters
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
                        //The maximum number of words to consider when making phrases
                        uint8_t m_max_phrase_len;
                    } de_parameters;
                }
            }
        }
    }
}

#endif /* DEC_PARAMETERS_HPP */

