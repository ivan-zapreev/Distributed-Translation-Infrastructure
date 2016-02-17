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

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/decoder/stack/multi_state.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/tm/proxy/tm_query_proxy.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::tm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

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

                            /**
                             * The basic constructor
                             */
                            multi_stack(const de_parameters & params,
                                    acr_bool_flag is_stop,
                                    const string & source_sent,
                                    const tm_query_proxy & tm_query,
                                    const rm_query_proxy & rm_query)
                            : m_params(params), m_is_stop(is_stop), m_source_sent(source_sent),
                            m_tm_query(tm_query), m_rm_query(rm_query),
                            m_lm_query(lm_configurator::allocate_query_proxy()) {
                                LOG_DEBUG1 << "Created a multi stack with parameters: " << (string) m_params << END_LOG;
                                LOG_DEBUG1 << "The source sentence is: " << m_source_sent << END_LOG;
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_stack() {
                                //Dispose the language query object
                                lm_configurator::dispose_query_proxy(m_lm_query);
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
                             * Allows to extend the hypothesis
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
                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;
                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;

                            //Stores the reference to the source sentence
                            const string & m_source_sent;

                            //The language mode query proxy
                            const tm_query_proxy & m_tm_query;
                            //The language mode query proxy
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

