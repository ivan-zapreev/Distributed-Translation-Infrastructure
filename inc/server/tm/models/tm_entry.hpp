/* 
 * File:   tm_entry.hpp
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
 * Created on February 8, 2016, 10:04 AM
 */

#ifndef TM_ENTRY_HPP
#define TM_ENTRY_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

#include "common/utils/containers/fixed_size_hashmap.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::containers;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {
                    namespace models {

                        //Declare the phrase unique identifier type
                        typedef uint64_t phrase_uid;

                        //Define the undefined phrase id value
                        static constexpr uint64_t UNDEFINED_PHRASE_ID = 0;

                        /**
                         * This structure represents the translation data, i.e. the
                         * the target phrase plus the probability weights. See:
                         * http://www.statmt.org/moses/?n=FactoredTraining.ScorePhrases
                         * for more details on the weights. Note that for this entry
                         * we have a uid that is a unique identifier of the target
                         * phrase string. The latter can be a hash value but then
                         * there is a possibility for the hash collisions
                         */
                        struct tm_target_entry {
                            //Stores the target phrase of the translation which a key value
                            string m_target_phrase;
                            //Stores the unique identifier of the given phrase
                            phrase_uid m_phrase_uid;
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

                            /**
                             * The basic constructor
                             */
                            tm_target_entry()
                            : m_target_phrase(""), m_phrase_uid(UNDEFINED_PHRASE_ID), m_sct_prob(0.0),
                            m_sct_lex(0.0), m_tcs_prob(0.0), m_tcs_lex(0.0) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_target_entry() {
                                //Clear the entry if it has not been cleared yet.
                                clear(*this);
                            }

                            /**
                             * The comparison operator, allows to compare translation entries
                             * @param phrase_uid the unique identifier of the translation entry to compare with
                             * @return true if the provided uid is equal to the uid of this entry, otherwise false 
                             */
                            inline bool operator==(const phrase_uid & phrase_uid) const {
                                return (m_phrase_uid == phrase_uid);
                            }

                            /**
                             * Allows to clear the data allocated for the given element
                             * @param elem the element to clear
                             */
                            static inline void clear(tm_target_entry & elem) {
                                //Nothing to be done, no dynamically allocated resources
                            }
                        };

                        //Define the translations data map. It represents possible translations for some source phrase.
                        typedef fixed_size_hashmap<tm_target_entry, const phrase_uid &> tm_target_entry_map;

                        /**
                         * This is the source entry data structure that contains two things
                         * The source phrase uid, which is the unique identifier of the
                         * source string and the map storing the target translations.
                         * Note that the source phrase is not stored, this is to reduce
                         * memory consumption and improve speed. Similar as we did for
                         * the g2dm tried implementation for the language model.
                         */
                        struct tm_source_entry {
                            //Stores the unique identifier of the given source
                            phrase_uid m_phrase_uid;
                            //Stores the target entries map pointer
                            tm_target_entry_map * m_targets;

                            /**
                             * The basic constructor
                             */
                            tm_source_entry()
                            : m_phrase_uid(UNDEFINED_PHRASE_ID), m_targets(NULL) {
                            }

                            /**
                             * The basic destructor
                             */
                            ~tm_source_entry() {
                                //Clear the entry if it has not been cleared yet.
                                clear(*this);
                            }

                            /**
                             * The comparison operator, allows to compare source entries
                             * @param phrase_uid the unique identifier of the source entry to compare with
                             * @return true if the provided uid is equal to the uid of this entry, otherwise false 
                             */
                            inline bool operator==(const phrase_uid & phrase_uid) const {
                                return (m_phrase_uid == phrase_uid);
                            }

                            /**
                             * Allows to clear the data allocated for the given element
                             * @param elem the element to clear
                             */
                            static inline void clear(tm_source_entry & elem) {
                                if (elem.m_targets != NULL) {
                                    delete elem.m_targets;
                                    elem.m_targets = NULL;
                                }
                            }
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_ENTRY_HPP */
