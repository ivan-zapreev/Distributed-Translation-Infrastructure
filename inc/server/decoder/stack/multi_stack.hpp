/* 
 * File:   trans_stack.hpp
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
 * Created on February 16, 2016, 4:20 PM
 */

#ifndef TRANS_STACK_HPP
#define TRANS_STACK_HPP

#include <string>
#include <queue>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/sentence/sentence_data_map.hpp"
#include "server/decoder/stack/multi_state.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

using namespace uva::smt::bpbd::server::decoder::sentence;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        /**
                         * This is the translation stack class that is responsible for the sentence translation
                         */
                        class multi_stack {
                        public:
                            //Define the stack type
                            typedef priority_queue<multi_state_ptr> stack_type;

                            /**
                             * The basic constructor
                             * @param num_words the number of words in the sentence
                             * @param params the decoder parameters, stores the reference to it
                             * @param is_stop the stop flag
                             * @param sent_data the retrieved sentence data
                             * @param rm_query the reordering model query
                             */
                            multi_stack(const de_parameters & params,
                                    acr_bool_flag is_stop,
                                    const sentence_data_map & sent_data,
                                    const rm_query_proxy & rm_query)
                            : m_root_state(params), m_stacks(NULL), m_params(params),
                            m_is_stop(is_stop), m_sent_data(sent_data), m_rm_query(rm_query),
                            m_lm_query(lm_configurator::allocate_query_proxy()) {
                                LOG_DEBUG1 << "Created a multi stack with parameters: " << (string) m_params << END_LOG;
                                //Instantiate the proper number of stacks, the same number as
                                //there is words plus one. The last stack is for </s> words.
                                m_stacks = new stack_type[m_sent_data.get_dim() + 1]();
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_stack() {
                                //Dispose the language query object
                                lm_configurator::dispose_query_proxy(m_lm_query);
                                //Dispose the stacks
                                if (m_stacks != NULL) {
                                    delete[] m_stacks;
                                    m_stacks = NULL;
                                }
                            }

                            /**
                             * Allows to check if the translation process is finished
                             * @return true if the translation is dinished
                             */
                            bool has_finished() {
                                //ToDo: Implement
                                return true;
                            }

                            /**
                             * Allows to extend the hypothesis, when extending the stack we immediately re-combine
                             */
                            void extend() {
                                //ToDo: Implement
                            }

                            /**
                             * Allows to prune the hypothesis
                             */
                            void prune() {
                                //ToDo: Implement
                            }

                            /**
                             * Allows to get the best translation from the
                             * stack after the decoding has finished.
                             * @param target_sent [out] the variable to store the translation
                             */
                            void get_best_translation(string & target_sent) {
                                //ToDo: Implement
                            }

                        protected:

                        private:
                            //Stores the root multi-stack state element
                            multi_state m_root_state;
                            //Stores the stacks containing ordered states
                            stack_type * m_stacks;

                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;
                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;

                            //The reference to the sentence data map
                            const sentence_data_map & m_sent_data;
                            //The reference to the reordering mode query data
                            const rm_query_proxy & m_rm_query;

                            //Sores the language mode query proxy
                            lm_trie_query_proxy & m_lm_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_HPP */

