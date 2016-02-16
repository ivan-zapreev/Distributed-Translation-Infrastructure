/* 
 * File:   trans_stack_state.hpp
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

#ifndef TRANS_STACK_STATE_HPP
#define TRANS_STACK_STATE_HPP

#include <vector>
#include <bitset>

#include "common/utils/containers/circular_queue.hpp"

#include "server/decoder/de_configs.hpp"

using namespace std;

using namespace uva::utils::containers;

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {
                    namespace stack {

                        //Typedef the string pointer
                        typedef string * string_ptr;
                        
                        /**
                         * This is the translation stack state class that is responsible for the sentence translation
                         */
                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_TARGET_PHRASE_LENGTH>
                        class multi_state {
                        public:

                        //Define the stack state pointer
                        typedef multi_state<NUM_WORDS_PER_SENTENCE, MAX_TARGET_PHRASE_LENGTH> * multi_state_ptr;

                            //Stores the undefined word index
                            static constexpr int32_t UNDEFINED_WORD_IDX = -1;
                            static constexpr int32_t ZERRO_WORD_IDX = UNDEFINED_WORD_IDX + 1;

                            /**
                             * The basic constructor for the root stack state
                             */
                            multi_state()
                            : m_parent(NULL), m_next(NULL), m_recomb_from(), m_recomb_to(NULL),
                            m_covered(), m_last_covered(ZERRO_WORD_IDX), m_history(), m_prob(0.0) {
                                //Mark the zero word as covered
                                m_covered.set(ZERRO_WORD_IDX);
                                //Add the sentence start to the target
                                m_history.push_back(BEGIN_SENTENCE_TAG_STR);
                            }

                            /**
                             * The basic constructor for the non-root stack state
                             * @param parent the pointer to the parent element
                             */
                            multi_state(multi_state_ptr parent)
                            : m_parent(NULL), m_next(NULL), m_recomb_from(), m_recomb_to(NULL),
                            m_covered(), m_last_covered(UNDEFINED_WORD_IDX), m_history(), m_prob(0.0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~multi_state() {
                            }

                        private:

                        protected:
                            //This variable stores the pointer to the parent state or NULL if it is the root state
                            multi_state_ptr m_parent;

                            //This variable stores the pointer to the next state in the stack or NULL if it is the last one
                            multi_state_ptr m_next;

                            //This vector stores the list of states recombined into this state
                            vector<multi_state_ptr> m_recomb_from;
                            //This variable stores to which state this state was recombined or NULL
                            multi_state_ptr m_recomb_to;

                            //Stores the bitset of covered words indexes
                            bitset<NUM_WORDS_PER_SENTENCE> m_covered;

                            //Stores the last translated word index
                            int32_t m_last_covered;

                            //Stores the N-1 previously translated words
                            circular_queue<string_ptr, MAX_TARGET_PHRASE_LENGTH - 1 > m_history;

                            //Stores the logarithmic probability weight
                            float m_prob;
                        };

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_TARGET_PHRASE_LENGTH>
                        constexpr int32_t multi_state<NUM_WORDS_PER_SENTENCE, MAX_TARGET_PHRASE_LENGTH>::UNDEFINED_WORD_IDX;

                        template<size_t NUM_WORDS_PER_SENTENCE, size_t MAX_TARGET_PHRASE_LENGTH>
                        constexpr int32_t multi_state<NUM_WORDS_PER_SENTENCE, MAX_TARGET_PHRASE_LENGTH>::ZERRO_WORD_IDX;
                    }
                }
            }
        }
    }
}

#endif /* TRANS_STACK_STATE_HPP */

