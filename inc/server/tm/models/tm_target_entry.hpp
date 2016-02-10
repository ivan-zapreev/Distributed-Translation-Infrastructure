/* 
 * File:   tm_target_entry.hpp
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
 * Created on February 9, 2016, 5:35 PM
 */

#ifndef TM_TARGET_ENTRY_HPP
#define TM_TARGET_ENTRY_HPP

#include <string>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/hashing_utils.hpp"

#include "server/tm/models/tm_phrase_id.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::hashing;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    namespace models {

                        /**
                         * This structure represents the translation data, i.e. the
                         * the target phrase plus the probability weights. See:
                         * http://www.statmt.org/moses/?n=FactoredTraining.ScorePhrases
                         * for more details on the weights. Note that for this entry
                         * we have a uid that is a unique identifier of the target
                         * phrase string. The latter can be a hash value but then
                         * there is a possibility for the hash collisions
                         */
                        class tm_target_entry {
                        public:

                            /**
                             * The basic constructor
                             */
                            tm_target_entry()
                            : m_target_phrase(""), m_sct_prob(0.0),
                            m_sct_lex(0.0), m_tcs_prob(0.0), m_tcs_lex(0.0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_target_entry() {
                                //Nothing to clean everything is stack allocated.
                            }

                            /**
                             * Allows to set the target phrase and its id
                             * @param target_phrase the target phrase
                             */
                            inline void set_target(string target_phrase) {
                                m_target_phrase = target_phrase;
                            }

                            /**
                             * Allows to get the reference to the inverse phrase translation probability φ(f|e)
                             * @return the reference to the inverse phrase translation probability φ(f|e)
                             */
                            inline float & get_sct_prob() {
                                return m_sct_prob;
                            }

                            /**
                             * Allows to get the reference to the inverse lexical weighting lex(f|e)
                             * @return the reference to the inverse lexical weighting lex(f|e)
                             */
                            inline float & get_sct_lex() {
                                return m_sct_lex;
                            }

                            /**
                             * Allows to get the reference to the direct phrase translation probability φ(e|f)
                             * @return the reference to the direct phrase translation probability φ(e|f)
                             */
                            inline float & get_tcs_prob() {
                                return m_tcs_prob;
                            }

                            /**
                             * Allows to get the reference to the direct lexical weighting lex(e|f)
                             * @return the reference to the direct lexical weighting lex(e|f)
                             */
                            inline float & get_tcs_lex() {
                                return m_tcs_lex;
                            }

                        private:
                            //Stores the target phrase of the translation which a key value
                            string m_target_phrase;
                            //The conditional probability value for source conditioned on target
                            float m_sct_prob;
                            //Inverse lexical weighting lex(f|e)
                            //ToDo: Do we need it for decoding?
                            float m_sct_lex;
                            //The conditional probability value for target conditioned on source
                            float m_tcs_prob;
                            //Direct lexical weighting lex(e|f)
                            //ToDo: Do we need it for decoding?
                            float m_tcs_lex;
                            //Phrase penalty (always exp(1) = 2.718) therefore is static
                            //ToDo: Do we need it while decoding?
                            static float m_phrase_penalty;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_TARGET_ENTRY_HPP */

