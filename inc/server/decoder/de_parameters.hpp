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
#include <map>
#include <utility>
#include <cmath>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/threads/threads.hpp"

#include "server/decoder/de_configs.hpp"
#include "server/server_consts.hpp"
#include "server/common/feature_id_registry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::threads;

using namespace uva::smt::bpbd::server::common;
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
                        //Stores the configuration section name
                        static const string DE_CONFIG_SECTION_NAME;
                        //The pruning threshold parameter name
                        static const string DE_PRUNING_THRESHOLD_PARAM_NAME;
                        //The stack level capacity parameter name
                        static const string DE_STACK_CAPACITY_PARAM_NAME;
                        //The maximum source phrase length parameter name
                        static const string DE_MAX_SP_LEN_PARAM_NAME;
                        //The maximum target phrase length parameter name
                        static const string DE_MAX_TP_LEN_PARAM_NAME;
                        //The linear distortion weight parameter name
                        static const string DE_DIST_LIMIT_PARAM_NAME;
                        //The distortion limit parameter name
                        static const string DE_LD_PENALTY_PARAM_NAME;

                        //The is-generate-search-lattice parameter name
                        static const string DE_IS_GEN_LATTICE_PARAM_NAME;
                        //The lattices folder parameter name
                        static const string DE_LATTICES_FOLDER_PARAM_NAME;
                        //The lattice id to name mapping file extension parameter name
                        static const string DE_LI2N_FILE_EXT_PARAM_NAME;
                        //The feature scores file extension parameter name
                        static const string DE_SCORES_FILE_EXT_PARAM_NAME;
                        //The lattice file parameter name
                        static const string DE_LATTICE_FILE_EXT_PARAM_NAME;

                        //The global id of the linear distortion feature
                        static size_t DE_LD_PENALTY_GLOBAL_ID;

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
                        //The logarithm value of the pruning threshold
                        atomic<float> m_pruning_threshold_log;
                        //The stack capacity for stack pruning
                        atomic<uint32_t> m_stack_capacity;
                        //Stores the linear distortion lambda parameter value
                        atomic<float> m_lin_dist_penalty;

                        //This flag is needed for when the server is compiled in the tuning mode.
                        //This flag should allow to set the tuning lattice generation of and off.
                        a_bool_flag m_is_gen_lattice;
                        //The server configuration file name, is only set if IS_SERVER_TUNING_MODE == true
                        string m_config_file_name;
                        //The folder where the lattice related files are to be stored
                        string m_lattices_folder;
                        //The lattice id to name file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_li2n_file_ext;
                        //The lattice feature scores file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_scores_file_ext;
                        //The lattice file extension, is only set if IS_SERVER_TUNING_MODE == true
                        string m_lattice_file_ext;
                        //Stores the number of known features, for the case of lattice generation
                        //This is not to be output with the << operator.
                        size_t m_num_features;

                        /**
                         * Store the feature ids in a form of an enumeration
                         */
                        enum de_weight_ids {
                            DE_WORD_PENALTY_ID = 0,
                            DE_PHRASE_PENALTY_ID = DE_WORD_PENALTY_ID + 1,
                            DE_LIN_DIST_PENALTY_ID = DE_PHRASE_PENALTY_ID + 1,
                            de_weight_ids_size = DE_LIN_DIST_PENALTY_ID + 1
                        };

                        /**
                         * Allows to get the features weights used in the corresponding model.
                         * @param registry the feature registry entity
                         */
                        inline void get_weight_names(feature_id_registry & registry) {
                            //Declare the feature source 
                            const string source = __FILENAME__;
                            //Add the feature weight names and increment the weight count
                            registry.add_feature(source, DE_LD_PENALTY_PARAM_NAME, DE_LD_PENALTY_GLOBAL_ID);
                        }

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
                                this->m_pruning_threshold = other.m_pruning_threshold.load();
                                this->m_pruning_threshold_log = other.m_pruning_threshold_log.load();
                                this->m_stack_capacity = other.m_stack_capacity.load();
                                this->m_lin_dist_penalty = other.m_lin_dist_penalty.load();
                                this->m_is_gen_lattice = other.m_is_gen_lattice.load();
                                this->m_lattices_folder = other.m_lattices_folder;
                                this->m_li2n_file_ext = other.m_li2n_file_ext;
                                this->m_scores_file_ext = other.m_scores_file_ext;
                                this->m_lattice_file_ext = other.m_lattice_file_ext;
                                this->m_num_features = other.m_num_features;
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
                            ASSERT_CONDITION_THROW((m_max_s_phrase_len == 0),
                                    string("The ") + DE_MAX_SP_LEN_PARAM_NAME + string(" must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len == 0),
                                    string("The ") + DE_MAX_TP_LEN_PARAM_NAME + string(" must not be 0!"));

                            ASSERT_CONDITION_THROW((m_max_t_phrase_len > tm::TM_MAX_TARGET_PHRASE_LEN),
                                    string("The ") + DE_MAX_TP_LEN_PARAM_NAME + string(" must not be <= ") +
                                    to_string(tm::TM_MAX_TARGET_PHRASE_LEN));

                            ASSERT_CONDITION_THROW(((m_pruning_threshold <= 0.0) || (m_pruning_threshold >= 1.0)),
                                    string("The ") + DE_PRUNING_THRESHOLD_PARAM_NAME +
                                    string(" must be within an open interval (0.0, 1.0)!"));

                            //Compute the log value of the pruning threshold
                            m_pruning_threshold_log = std::log(m_pruning_threshold);

                            ASSERT_CONDITION_THROW((m_stack_capacity <= 0),
                                    string("The ") + DE_STACK_CAPACITY_PARAM_NAME +
                                    string(" must be > 0!"));

                            if (this->m_is_gen_lattice) {
#if IS_SERVER_TUNING_MODE
                                //Check if the lattices folder is set
                                if (m_lattices_folder == "") {
                                    //Log the warning
                                    LOG_WARNING << "The " << DE_LATTICES_FOLDER_PARAM_NAME
                                            << " is set to EMPTY in a training mode "
                                            << ", re-setting to current ('.')!" << END_LOG;
                                    //just use the current folder
                                    m_lattices_folder = ".";
                                }

                                ASSERT_CONDITION_THROW((m_li2n_file_ext == ""),
                                        string("The ") + DE_LI2N_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
                                ASSERT_CONDITION_THROW((m_scores_file_ext == ""),
                                        string("The ") + DE_SCORES_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
                                ASSERT_CONDITION_THROW((m_lattice_file_ext == ""),
                                        string("The ") + DE_LATTICE_FILE_EXT_PARAM_NAME + string(" must not be empty!"));
                                ASSERT_CONDITION_THROW((m_num_features == 0),
                                        string("The total number of translation features: ") +
                                        to_string(m_num_features) +
                                        string(", must be larger than zero!"));
                                ASSERT_CONDITION_THROW((m_num_features > MAX_NUMBER_OF_REATURES),
                                        string("The total number of translation features: ") +
                                        to_string(m_num_features) +
                                        string(" exceeds the allowed maximum of: ") +
                                        to_string(MAX_NUMBER_OF_REATURES));
#else
                                LOG_WARNING << "The " << DE_IS_GEN_LATTICE_PARAM_NAME
                                        << " is set to TRUE in a non-training mode server"
                                        << " compilation, re-setting to FALSE!" << END_LOG;
                                this->m_is_gen_lattice = false;
#endif
                            }
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
                        stream << "DE parameters: { ";

                        //Log the distortion limit parameter
                        stream << de_parameters::DE_DIST_LIMIT_PARAM_NAME << " = ";
                        if (params.m_dist_limit >= 0) {
                            stream << params.m_dist_limit;
                        } else {
                            stream << "none (help)";
                        }

                        //The linear distortion penalty lambda
                        stream << ", " << de_parameters::DE_LD_PENALTY_PARAM_NAME << " = " << params.m_lin_dist_penalty;

                        //Log simple value parameters
                        stream << ", " << de_parameters::DE_PRUNING_THRESHOLD_PARAM_NAME << " = " << params.m_pruning_threshold
                                << ", " << de_parameters::DE_STACK_CAPACITY_PARAM_NAME << " = " << params.m_stack_capacity
                                << ", " << de_parameters::DE_MAX_SP_LEN_PARAM_NAME << " = " << to_string(params.m_max_s_phrase_len)
                                << ", " << de_parameters::DE_MAX_TP_LEN_PARAM_NAME << " = " << to_string(params.m_max_t_phrase_len)
                                << ", " << de_parameters::DE_IS_GEN_LATTICE_PARAM_NAME << " = " << (params.m_is_gen_lattice ? "true" : "false");

                        //Log the additional lattice related parameters, if needed
                        if (params.m_is_gen_lattice) {
                            stream << ", " << de_parameters::DE_LI2N_FILE_EXT_PARAM_NAME << " = '." << params.m_li2n_file_ext << "'"
                                    << ", " << de_parameters::DE_SCORES_FILE_EXT_PARAM_NAME << " = '." << params.m_scores_file_ext << "'"
                                    << ", " << de_parameters::DE_LATTICE_FILE_EXT_PARAM_NAME << " = '." << params.m_lattice_file_ext << "'";
                        }

                        return stream << " }";
                    }
                }
            }
        }
    }
}

#endif /* DEC_PARAMETERS_HPP */

