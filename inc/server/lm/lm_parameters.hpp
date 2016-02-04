/* 
 * File:   language_model_parameters.hpp
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
 * Created on February 4, 2016, 11:52 AM
 */

#ifndef LANGUAGE_MODEL_PARAMETERS_HPP
#define LANGUAGE_MODEL_PARAMETERS_HPP

#include "trie_configs.hpp"

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {

                    /**
                     * This structure is needed to store the language model parameters
                     */
                    typedef struct {
                        //The train file name, is defined by the used
                        string m_model_file_name;

                        //The Trie type name, is defined by the user
                        string m_trie_type_name;

                        //The word index type to be used with the trie, is
                        //defined by the implementation from the used input
                        WordIndexTypesEnum m_word_index_type;

                        //Stores the word index memory factor, is defined
                        //by the implementation
                        float m_word_index_mem_fact;

                        //The trie type, is defined by the implementation
                        //from the used input
                        TrieTypesEnum m_trie_type;
                    } lm_parameters;
                }
            }
        }
    }
}

#endif /* LANGUAGE_MODEL_PARAMETERS_HPP */

