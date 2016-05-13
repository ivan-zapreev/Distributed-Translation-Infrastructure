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
#include "common/utils/threads.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/server_consts.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::tm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

                    /**
                     * This structure stores the decoder parameters
                     */
                    struct de_parameters_struct {
                        //The distortion limit to use; <integer>
                        //The the number of words to the right and left
                        //from the last phrase end word to consider
                        atomic<int32_t> m_dist_limit;

                        //The maximum number of words to consider when making phrases
                        phrase_length m_max_s_phrase_len;
                        //The maximum number of words to consider when making phrases
                        phrase_length m_max_t_phrase_len;

                        //The pruning threshold is to be a <positive float> it is 
                        //the %/100 deviation from the best hypothesis score. 
                        atomic<float> m_pruning_threshold;
                        //The pruning multiplier for positive costs, is computed from the 
                        //m_pruning_threshold automatically in the finalization function
                        atomic<float> m_pruning_mult_neg;
                        //The pruning multiplier for negative costs, is computed from the 
                        //m_pruning_threshold automatically in the finalization function
                        atomic<float> m_pruning_mult_pos;
                        //The stack capacity for stack pruning
                        atomic<uint32_t> m_stack_capacity;
                        //Stores the word penalty - the cost of each target word
                        atomic<float> m_word_penalty;
                        //Stores the phrase penalty - the cost of each target phrase
                        atomic<float> m_phrase_penalty;
                        //Stores the linear distortion lambda parameter value
                        atomic<float> m_lin_dist_penalty;

                        //Stores the number of best translations 
                        atomic<uint32_t> m_num_best_trans;

                        //Stores the number of the alternative translations/hypothesis
                        //for a state to keep. This value is used when two states are
                        //recombined. If the value is zero then we only keep one of the
                        //two equivalent hypothesis, with the highest score. 
                        atomic<uint32_t> m_num_alt_to_keep;

                        //This flag is needed for when the server is compiled in the tuning mode.
                        //This flag should allow to set the tuning lattice generation of and off.
                        atomic<bool> m_is_gen_lattice;
                        //The server configuration file name, is only set if IS_SERVER_TUNING_MODE == true
                        string m_config_file_name;
                        //The lattice id to name file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_li2d_file_ext;
                        //The lattice feature scores file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_scores_file_ext;
                        //The lattice file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_lattice_file_ext;

                        /**
                         * The basic constructor, does nothing
                         */
                        de_parameters_struct() {
                        }

                        /**
                         * The assignment operator
                         * @param other the object to assign from
                         * @return this object updated with new values
                         */
                        de_parameters_struct& operator=(const de_parameters_struct & other) {
                            if (this != &other) {
                                this->m_dist_limit = other.m_dist_limit.load();
                                this->m_max_s_phrase_len = other.m_max_s_phrase_len;
                                this->m_max_t_phrase_len = other.m_max_t_phrase_len;
                                this->m_num_best_trans = other.m_num_best_trans.load();
                                this->m_num_alt_to_keep = other.m_num_alt_to_keep.load();
                                this->m_phrase_penalty = other.m_phrase_penalty.load();
                                this->m_pruning_threshold = other.m_pruning_threshold.load();
                                this->m_pruning_mult_neg = other.m_pruning_mult_neg.load();
                                this->m_pruning_mult_pos = other.m_pruning_mult_pos.load();
                                this->m_stack_capacity = other.m_stack_capacity.load();
                                this->m_word_penalty = other.m_word_penalty.load();
                                this->m_lin_dist_penalty = other.m_lin_dist_penalty.load();
                                this->m_is_gen_lattice = other.m_is_gen_lattice.load();
                                this->m_config_file_name = other.m_config_file_name;
                                this->m_li2d_file_ext = other.m_li2d_file_ext;
                                this->m_scores_file_ext = other.m_scores_file_ext;
                                this->m_lattice_file_ext = other.m_lattice_file_ext;
                            }

                            return *this;
                        }

                        /**
                         * The copy constructor
                         * @param other the object to construct from
                         */
                        de_parameters_struct(const de_parameters_struct & other) {
                            *this = other;
                        }

                        /**
                         * Allows to verify the parameters to be correct.
                         */
                        void finalize() {
                            ASSERT_CONDITION_THROW((m_dist_limit < 0),
                                    string("The ") + RM_DIST_LIMIT_PARAM_NAME + string(" must not be >= 0!"));

                            ASSERT_CONDITION_THROW((m_max_s_phrase_len == 0),
                                    string("The ") + DE_MAX_SP_LEN_PARAM_NAME + string(" must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len == 0),
                                    string("The ") + DE_MAX_TP_LEN_PARAM_NAME + string(" must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len > tm::TM_MAX_TARGET_PHRASE_LEN),
                                    string("The ") + DE_MAX_TP_LEN_PARAM_NAME + string(" must not be <= ") +
                                    to_string(tm::TM_MAX_TARGET_PHRASE_LEN));

                            ASSERT_CONDITION_THROW((m_pruning_threshold <= 0.0),
                                    string("The ") + DE_PRUNING_THRESHOLD_PARAM_NAME +
                                    string(" must be > 0.0!"));

                            //Set the multiplier for the negative value
                            m_pruning_mult_neg = 1.0 + m_pruning_threshold;
                            //Set the multiplier for the positive value
                            m_pruning_mult_pos = 1.0 - m_pruning_threshold;

                            ASSERT_CONDITION_THROW((m_stack_capacity <= 0),
                                    string("The ") + DE_STACK_CAPACITY_PARAM_NAME +
                                    string(" must be > 0!"));

                            ASSERT_CONDITION_THROW((m_num_best_trans < 1),
                                    string("The ") + DE_NUM_BEST_TRANS_PARAM_NAME +
                                    string(" must be >= 1!"));

                            //The number of alternative translations in
                            //the number of best translations minus one
                            this->m_num_alt_to_keep = m_num_best_trans - 1;

#if IS_SERVER_TUNING_MODE
                            ASSERT_CONDITION_THROW((m_li2d_file_ext == ""),
                                    string("The ") + DE_LI2N_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
                            ASSERT_CONDITION_THROW((m_scores_file_ext == ""),
                                    string("The ") + DE_SCORES_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
                            ASSERT_CONDITION_THROW((m_lattice_file_ext == ""),
                                    string("The ") + DE_LATTICE_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
#else
                            if (this->m_is_gen_lattice) {
                                LOG_WARNING << "The " << DE_IS_GEN_LATTICE_PARAM_NAME
                                        << " is set to true in a non-training mode server"
                                        << " compilation, re-setting to false!" << END_LOG;
                                this->m_is_gen_lattice = false;
                            }
#endif
                        }
                    };

                    //Typedef the structure
                    typedef de_parameters_struct de_parameters;

                    /**
                     * Allows to output the parameters object to the stream
                     * @param stream the stream to output into
                     * @param params the parameters object
                     * @return the stream that we output into
                     */
                    static inline std::ostream& operator<<(std::ostream& stream, const de_parameters & params) {
                        stream << "DE parameters: [ ";
                        //Log the number of best translations value
                        stream << DE_NUM_BEST_TRANS_PARAM_NAME << " = " << params.m_num_best_trans;

                        //Log the distortion limit parameter
                        stream << ", " << RM_DIST_LIMIT_PARAM_NAME << " = ";
                        if (params.m_dist_limit != 0) {
                            stream << params.m_dist_limit;
                        } else {
                            stream << "none (help)";
                        }

                        //The linear distortion penalty lambda
                        stream << ", " << RM_LIN_DIST_PARAM_NAME << " = " << params.m_lin_dist_penalty;

                        //Log simple value parameters
                        stream << ", " << DE_PRUNING_THRESHOLD_PARAM_NAME << " = " << params.m_pruning_threshold
                                << ", " << DE_STACK_CAPACITY_PARAM_NAME << " = " << params.m_stack_capacity
                                << ", " << TM_WORD_PENALTY_PARAM_NAME << " = " << params.m_word_penalty
                                << ", " << TM_PHRASE_PENALTY_PARAM_NAME << " = " << params.m_phrase_penalty
                                << ", " << DE_MAX_SP_LEN_PARAM_NAME << " = " << to_string(params.m_max_s_phrase_len)
                                << ", " << DE_MAX_TP_LEN_PARAM_NAME << " = " << to_string(params.m_max_t_phrase_len)
                                << ", " << DE_IS_GEN_LATTICE_PARAM_NAME << " = " << (params.m_is_gen_lattice ? "true" : "false");

                        //Log the additional lattice related parameters, if needed
                        if (params.m_is_gen_lattice) {
                            stream << ", " << DE_LI2N_FILE_EXT_PARAM_NAME << " = '." << params.m_li2d_file_ext << "'"
                                    << ", " << DE_SCORES_FILE_EXT_PARAM_NAME << " = '." << params.m_scores_file_ext << "'"
                                    << ", " << DE_LATTICE_FILE_EXT_PARAM_NAME << " = '." << params.m_lattice_file_ext << "'";
                        }

                        return stream << " ]";
                    }
                }
            }
        }
    }
}

#endif /* DEC_PARAMETERS_HPP */

