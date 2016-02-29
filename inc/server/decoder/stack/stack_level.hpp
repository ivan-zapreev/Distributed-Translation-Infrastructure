/* 
 * File:   multi_level.hpp
 * Author: zapreevis
 *
 * Created on February 17, 2016, 4:41 PM
 */

#ifndef STACK_LEVEL_HPP
#define	STACK_LEVEL_HPP

#include <string>

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/decoder/de_parameters.hpp"

#include "server/decoder/stack/stack_state.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {
                        
                        //Forward declaration of the stack level class
                        class stack_level;
                        //The typedef of the stack level pointer
                        typedef stack_level * stack_level_ptr;

                        /**
                         * Represents the multi-stack level
                         */
                        class stack_level {
                        public:

                            /**
                             * The basic constructor
                             * @param params the decoder parameters, stores the reference to it
                             * @param is_stop the stop flag
                             */
                            stack_level(const de_parameters & params, acr_bool_flag is_stop)
                            : m_params(params), m_is_stop(is_stop), m_first_state(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~stack_level() {
                                //ToDo: Iterate through the level states and delete them
                            }

                            /**
                             * Allows to add a new state into the level
                             * @param new_state the new state to add
                             */
                            void add_state(stack_state_ptr new_state) {
                                //Initialize the reference to the pointer to the
                                //state place where we should put the new one
                                stack_state_ptr & place_state = m_first_state;

                                //ToDo: Search through the list for two things:
                                // 1. The equal hypothesis == - for recombination
                                // 2. The order position < for cost ordering.
                                //In case the equal hypothesis is not found and
                                //the recombination is not done, then insert the
                                //hypothesis to the found position based on cost
                                //comparison.

                                //Search for the proper position of the state,
                                //or until we find an empty position, then stop.
                                while ((place_state != NULL) && (*new_state < *place_state)) {
                                    //Move further to the next state
                                    place_state = place_state->get_next();
                                }

                                //ToDo: How can we combine it with recombination?
                                //We re-combine two hypothesis if:
                                //1. They cover the same words
                                //2. They have the same last translated source word
                                //3. They have the same history: last n-1 target words

                                //Check if the found free
                                if (place_state == NULL) {
                                    //Assign the state to the found position
                                    place_state = new_state;
                                } else {
                                    //If the place state is not NULL then
                                }
                            }

                            /**
                             * Allows to expand the stack elements, to do that this method just
                             * goes through all the stack elements one by one and expands them
                             */
                            void expand() {
                                if (!m_is_stop) {
                                    //ToDo: Implement
                                }
                                THROW_NOT_IMPLEMENTED();
                            }

                            /**
                             * Allows to get the best translation target string for this stack.
                             * To do that, it takes the first element/state in the level's ordered 
                             * by costs stack and asks it to unroll itself to give its translation.
                             * @param target_sent [out] the variable to store the translation
                             */
                            void get_best_trans(string & target_sent) {
                                if (!m_is_stop) {
                                    //ToDo: Implement
                                }
                                THROW_NOT_IMPLEMENTED();
                            }

                        protected:

                        private:
                            //Stores the reference to the decoder parameters
                            const de_parameters & m_params;
                            //Stores the stopping flag
                            acr_bool_flag m_is_stop;
                            
                            //Stores the pointer to the first level state
                            stack_state_ptr m_first_state;
                        };
                    }
                }
            }
        }
    }
}

#endif	/* MULTI_LEVEL_HPP */

