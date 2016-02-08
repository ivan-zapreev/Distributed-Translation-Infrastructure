/* 
 * File:   lm_config_utils.hpp
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
 * Created on February 4, 2016, 4:52 PM
 */

#ifndef LM_CONFIG_UTILS_HPP
#define LM_CONFIG_UTILS_HPP

#include "common/utils/monitore/statistics_monitore.hpp"

#include "server/lm/lm_parameters.hpp"

#include "server/lm/models/c2d_map_trie.hpp"
#include "server/lm/models/w2c_hybrid_trie.hpp"
#include "server/lm/models/c2w_array_trie.hpp"
#include "server/lm/models/w2c_array_trie.hpp"
#include "server/lm/models/c2d_hybrid_trie.hpp"
#include "server/lm/models/g2d_map_trie.hpp"
#include "server/lm/models/h2d_map_trie.hpp"

using namespace uva::utils::monitore;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace __configurator {

                        //Initialize constants
                        static const string TC2DMapTrie_STR = string("c2dm");
                        static const string TW2CHybridTrie_STR = string("w2ch");
                        static const string TC2WArrayTrie_STR = string("c2wa");
                        static const string TW2CArrayTrie_STR = string("w2ca");
                        static const string C2DHybridTrie_STR = string("c2dh");
                        static const string G2DMapTrie_STR = string("g2dm");
                        static const string H2DMapTrie_STR = string("h2dm");

                        /**
                         * Returns the default trie type name string
                         * @return the default trie type name string
                         */
                        static inline string get_default_trie_type_str() {
                            //ToDo: Make configurable via the Configuration.h or Globals.h
                            return TC2WArrayTrie_STR;
                        }

                        /**
                         * Allows to get a string with all available (known to the factory) trie types
                         * @param p_supported_tries the pointer to the vector to be filled in with supported tries
                         */
                        static inline void get_trie_types_str(vector<string> * p_supported_tries) {
                            p_supported_tries->push_back(TC2DMapTrie_STR);
                            p_supported_tries->push_back(TW2CHybridTrie_STR);
                            p_supported_tries->push_back(TC2WArrayTrie_STR);
                            p_supported_tries->push_back(TW2CArrayTrie_STR);
                            p_supported_tries->push_back(C2DHybridTrie_STR);
                            p_supported_tries->push_back(G2DMapTrie_STR);
                            p_supported_tries->push_back(H2DMapTrie_STR);
                        }

                        /**
                         * Based on the trie name stored in the parameters allows
                         * to determine the trie type and the appropriate word
                         * index for the trie.
                         * @param params the parameters storing structure in/out
                         */
                        static inline void get_trie_and_word_index_types(lm_parameters& params) {
                            if (params.m_trie_type_name == TC2DMapTrie_STR) {
                                params.m_word_index_type = __C2DMapTrie::WORD_INDEX_TYPE;
                                params.m_trie_type = trie_types::C2DM_TRIE;
                            } else {
                                if (params.m_trie_type_name == TW2CHybridTrie_STR) {
                                    params.m_word_index_type = __W2CHybridTrie::WORD_INDEX_TYPE;
                                    params.m_trie_type = trie_types::W2CH_TRIE;
                                } else {
                                    if (params.m_trie_type_name == TC2WArrayTrie_STR) {
                                        params.m_word_index_type = __C2WArrayTrie::WORD_INDEX_TYPE;
                                        params.m_trie_type = trie_types::C2WA_TRIE;
                                    } else {
                                        if (params.m_trie_type_name == TW2CArrayTrie_STR) {
                                            params.m_word_index_type = __W2CArrayTrie::WORD_INDEX_TYPE;
                                            params.m_trie_type = trie_types::W2CA_TRIE;
                                        } else {
                                            if (params.m_trie_type_name == C2DHybridTrie_STR) {
                                                params.m_word_index_type = __C2DHybridTrie::WORD_INDEX_TYPE;
                                                params.m_trie_type = trie_types::C2DH_TRIE;
                                            } else {
                                                if (params.m_trie_type_name == G2DMapTrie_STR) {
                                                    params.m_word_index_type = __G2DMapTrie::WORD_INDEX_TYPE;
                                                    params.m_trie_type = trie_types::G2DM_TRIE;
                                                } else {
                                                    if (params.m_trie_type_name == H2DMapTrie_STR) {
                                                        params.m_word_index_type = __H2DMapTrie::WORD_INDEX_TYPE;
                                                        params.m_trie_type = trie_types::H2DM_TRIE;
                                                    } else {
                                                        THROW_EXCEPTION(string("Unrecognized trie type: ") + params.m_trie_type_name);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        //The number of bytes in one Mb
                        const uint32_t BYTES_ONE_MB = 1024u;

                        /**
                         * This function is meant to give the memory statistics information delta
                         * @param action the monitored action
                         * @param msStart the start memory usage statistics
                         * @param msEnd the end memory usage statistics
                         * @param isDoInfo true if the memory info may be print
                         */
                        static inline void report_memory_usage(const char* action, TMemotyUsage msStart, TMemotyUsage msEnd, const bool isDoInfo) {
                            LOG_USAGE << "Action: \'" << action << "\' memory change:" << END_LOG;
                            LOG_DEBUG << "\tmemory before: vmsize=" << SSTR(msStart.vmsize) << " Kb, vmpeak="
                                    << SSTR(msStart.vmpeak) << " Kb, vmrss=" << SSTR(msStart.vmrss)
                                    << " Kb, vmhwm=" << SSTR(msStart.vmhwm) << " Kb" << END_LOG;
                            LOG_DEBUG << "memory after: vmsize=" << SSTR(msEnd.vmsize) << " Kb, vmpeak="
                                    << SSTR(msEnd.vmpeak) << " Kb, vmrss=" << SSTR(msEnd.vmrss)
                                    << " Kb, vmhwm=" << SSTR(msEnd.vmhwm) << " Kb" << END_LOG;

                            int vmsize = ((msEnd.vmsize < msStart.vmsize) ? 0 : msEnd.vmsize - msStart.vmsize) / BYTES_ONE_MB;
                            int vmpeak = ((msEnd.vmpeak < msStart.vmpeak) ? 0 : msEnd.vmpeak - msStart.vmpeak) / BYTES_ONE_MB;
                            int vmrss = ((msEnd.vmrss < msStart.vmrss) ? 0 : msEnd.vmrss - msStart.vmrss) / BYTES_ONE_MB;
                            int vmhwm = ((msEnd.vmhwm < msStart.vmhwm) ? 0 : msEnd.vmhwm - msStart.vmhwm) / BYTES_ONE_MB;
                            LOG_USAGE << showpos << "vmsize=" << vmsize << " Mb, vmpeak=" << vmpeak
                                    << " Mb, vmrss=" << vmrss << " Mb, vmhwm=" << vmhwm
                                    << " Mb" << noshowpos << END_LOG;
                            if (isDoInfo) {
                                LOG_INFO << "  vmsize - Virtual memory size; vmpeak - Peak virtual memory size" << END_LOG;
                                LOG_INFO << "    Virtual memory size is how much virtual memory the process has in total (RAM+SWAP)" << END_LOG;
                                LOG_INFO << "  vmrss  - Resident set size; vmhwm  - Peak resident set size" << END_LOG;
                                LOG_INFO << "    Resident set size is how much memory this process currently has in main memory (RAM)" << END_LOG;
                            }
                        }
                    }
                }
            }
        }
    }
}

#endif /* LM_CONFIG_UTILS_HPP */

