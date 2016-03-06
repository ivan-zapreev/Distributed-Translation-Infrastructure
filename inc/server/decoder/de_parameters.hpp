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

#include <string>
#include <ostream>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "server/decoder/de_configs.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

                    /**
                     * This structure stores the decoder parameters
                     */
                    typedef struct {
                        //The distortion limit to use; <integer>
                        //The the number of words to the right and left
                        //from the last phrase end word to consider
                        int32_t m_distortion;
                        //Stores the flag indicating wether there is a left distortion limit
                        //or not, the value is set by this class in the finalize method
                        bool m_is_dist;

                        //The extra left distortion limit to use; <unsigned integer>
                        //The the number of words to the left of the last translated phrase
                        //begin word to consider in case that the regular distortion limit
                        //does not allow to jump left to the last translated phrase 
                        int32_t m_ext_dist_left;

                        //The maximum number of words to consider when making phrases
                        phrase_length m_max_s_phrase_len;
                        //The maximum number of words to consider when making phrases
                        phrase_length m_max_t_phrase_len;

                        //The pruning threshold is to be a <positive float> it is 
                        //the deviation from the best hypothesis score. I.e. must
                        //be a value from the half open interval [1.0, +inf)
                        float m_pruning_threshold;
                        //The stack capacity for stack pruning
                        uint32_t m_stack_capacity;
                        //Stores the word penalty - the cost of each target word
                        float m_word_penalty;
                        //Stores the phrase penalty - the cost of each target phrase
                        float m_phrase_penalty;
                        
                        //Stores the number of best translations we want to track
                        //This is the maximum number of states that we will keep
                        //in the recombination array for each state
                        size_t m_num_best_trans;

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            ASSERT_CONDITION_THROW((m_ext_dist_left < 0),
                                    string("The ext_dist_left must not be >= 0!"));

                            ASSERT_CONDITION_THROW((m_distortion == 0),
                                    string("The m_distortion must not be 0!"));
                            m_is_dist = (m_distortion > 0);

                            ASSERT_CONDITION_THROW((m_max_s_phrase_len == 0),
                                    string("The max_source_phrase_len must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len == 0),
                                    string("The max_target_phrase_len must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len > tm::TM_MAX_TARGET_PHRASE_LEN),
                                    string("The max_target_phrase_len must not be <= ") +
                                    to_string(tm::TM_MAX_TARGET_PHRASE_LEN));

                            ASSERT_CONDITION_THROW((m_pruning_threshold < 1.0),
                                    string("The pruning_threshold must be >= 1.0!"));

                            ASSERT_CONDITION_THROW((m_word_penalty == 0.0),
                                    string("The word_penalty must not be 0.0!"));

                            ASSERT_CONDITION_THROW((m_phrase_penalty == 0.0),
                                    string("The phrase_penalty must not be 0.0!"));

                            ASSERT_CONDITION_THROW((m_stack_capacity == 0),
                                    string("The stack_capacity must be > 0!"));

                            ASSERT_CONDITION_THROW((m_num_best_trans == 0),
                                    string("The num_best_trans must be > 0!"));
                        }
                    } de_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const de_parameters & params) {
                        return stream << "DE parameters: [ is_distortion = " << params.m_is_dist << ", distortion = "
                                << params.m_distortion << ", extra left distortion = " << params.m_ext_dist_left
                                << ", m_max_source_phrase_len = " << to_string(params.m_max_s_phrase_len)
                                << ", m_max_target_phrase_len = " << to_string(params.m_max_t_phrase_len)
                                << ", pruning_threshold = " << params.m_pruning_threshold
                                << ", stack_capacity = " << params.m_stack_capacity
                                << ", word_penalty = " << params.m_word_penalty
                                << ", phrase_penalty = " << params.m_phrase_penalty
                                << " ]";
                    }
                }
            }
        }
    }
}

#endif /* DEC_PARAMETERS_HPP */

