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

#ifndef MULTI_STACK_HPP
#define MULTI_STACK_HPP

#include <string>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/lm/lm_configurator.hpp"
#include "server/rm/proxy/rm_query_proxy.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"
#include "server/decoder/sentence/sentence_data_map.hpp"
#include "server/decoder/stack/stack_level.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::proxy;
using namespace uva::smt::bpbd::server::rm::proxy;

using namespace uva::smt::bpbd::server::decoder;
using namespace uva::smt::bpbd::server::decoder::sentence;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Stores the number of extrs stack levels that we will need in a multi-
                        //stack, the first one is for <s> and the second one is for </s>
                        static constexpr uint32_t NUM_EXTRA_STACK_LEVELS = 2;

                        /**
                         * This is the translation stack class that is responsible for the sentence translation
                         */
                        class multi_stack {
                        public:

                            /**
                             * The basic constructor
                             * @param params the decoder parameters, stores the reference to it
                             * @param is_stop the stop flag
                             * @param sent_data the retrieved sentence data
                             * @param rm_query the reordering model query
                             * @param lm_query the language model query object
                             */
                            multi_stack(const de_parameters & params,
                                    acr_bool_flag is_stop,
                                    const sentence_data_map & sent_data,
                                    const rm_query_proxy & rm_query,
                                    lm_fast_query_proxy & lm_query)
                            : m_params(params), m_is_stop(is_stop), m_sent_data(sent_data),
                            m_rm_query(rm_query), m_lm_query(lm_query),
                            m_num_levels(m_sent_data.get_dim() + NUM_EXTRA_STACK_LEVELS) {
                                LOG_DEBUG1 << "Created a multi stack with parameters: " << m_params << END_LOG;

                                //Instantiate the needed number of stack levels
                                m_levels = new stack_level[m_num_levels]();

                                //ToDo: Initialize the stack levels
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_stack() {
                                //Dispose the stacks
                                if (m_levels != NULL) {
                                    delete[] m_levels;
                                    m_levels = NULL;
                                }
                            }

                            /**
                             * Allows to extend the hypothesis, when extending the stack we immediately re-combine
                             */
                            void expand() {
                                if (!m_is_stop) {
                                    //ToDo: Implement
                                }
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * Allows to get the best translation from the
                             * stack after the decoding has finished.
                             * @param target_sent [out] the variable to store the translation
                             */
                            void get_best_translation(string & target_sent) const {
                                if (!m_is_stop) {
                                //ToDo: Implement
                                }
                                THROW_NOT_IMPLEMENTED();
                            }

                        protected:

                            /**
                             * Allows to check if the translation has been finished
                             * @return true if the translation has been finished, otherwise false
                             */
                            bool has_finished() const {
                                //ToDo: Implement
                                THROW_NOT_IMPLEMENTED();
                            }

                        private:
                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;
                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;

                            //The reference to the sentence data map
                            const sentence_data_map & m_sent_data;
                            //The reference to the reordering mode query data
                            const rm_query_proxy & m_rm_query;
                            //Sores the language mode query proxy
                            lm_fast_query_proxy & m_lm_query;

                            //Stores the number of multi-stack levels
                            const uint32_t m_num_levels;
                            //This is a pointer to the array of stacks, one stack per number of covered words.
                            stack_level * m_levels;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_HPP */

