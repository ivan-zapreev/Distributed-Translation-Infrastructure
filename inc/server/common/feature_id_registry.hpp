/* 
 * File:   feature_id_registry.hpp
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
 * Created on June 10, 2016, 9:56 AM
 */

#ifndef FEATURE_ID_REGISTRY_HPP
#define FEATURE_ID_REGISTRY_HPP

#include <map>
#include <string>
#include <fstream>

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace common {

                    /**
                     * This class is responsible for registering the model
                     * features and issuing them a feature id. The latter can be
                     * dumped in to an id-to-feature mapping file needed for lattice
                     * generation.
                     */
                    class feature_id_registry {
                    public:

                        /**
                         * The basic constructor
                         */
                        feature_id_registry() : m_next_feature_id(0), m_feature_2_id(), m_feature_2_source() {
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~feature_id_registry() {
                        }

                        /**
                         * Allows to register a new feature and to issue it a global issue id
                         * @param feature_name the feature name
                         * @param global_feature_id the issued feature id to be remembered
                         */
                        inline void add_feature(const string & source_name, const string & feature_name, size_t & global_feature_id) {
                            //Check that the feature with such a name has not been registered yet
                            auto prev_entry = m_feature_2_source.find(feature_name);
                            ASSERT_CONDITION_THROW((prev_entry != m_feature_2_source.end()),
                                    string("The feature: '") + feature_name +
                                    string("' has already been registered by source: '") +
                                    prev_entry->second + string("' the new source is: '") +
                                    source_name + string("'"));

                            //Assign the feature id;
                            global_feature_id = m_next_feature_id;
                            //Add the feature to the mapping 
                            m_feature_2_id[m_next_feature_id] = feature_name;
                            //Increment the next feature id
                            m_next_feature_id++;

                            LOG_USAGE << source_name << "::" << feature_name << " -> " << to_string(global_feature_id) << END_LOG;
                            
                            
                            //Remember the feature source
                            m_feature_2_source[feature_name] = source_name;
                        }

                        /**
                         * Allows to dump the feature id to name mapping into a file with the given name
                         * @param cfg_file_name the configuration file name
                         */
                        inline void dump_feature_to_id_file(const string & file_name) const {
                            //Open the output stream to the file
                            ofstream id2n_file(file_name);

                            //Check that the file is open
                            ASSERT_CONDITION_THROW(!id2n_file.is_open(), string("Could not open: ") +
                                    file_name + string(" for writing"));

                            //Iterate and output
                            for (auto iter = m_feature_2_id.begin(); iter != m_feature_2_id.end(); ++iter) {
                                //Output the mapping to the file
                                id2n_file << iter->second << "\t" << iter->first << std::endl;
                            }

                            //Close the file
                            id2n_file.close();

                            LOG_USAGE << "The feature id-to-name mapping is dumped into: " << file_name << END_LOG;
                        }
                        
                        /**
                         * Allows to get the number of registered features
                         * @return the number of registered features
                         */
                        size_t size() const {
                            return m_feature_2_id.size();
                        }
                        
                    private:
                        //Storrs the next feature id to be issued
                        size_t m_next_feature_id;

                        //Stores the mapping from the feature weight names to the ids.
                        map<size_t, string> m_feature_2_id;
                        //Stores the mapping from the feature weight names to the sources thereof.
                        map<string, string> m_feature_2_source;

                    };
                }
            }
        }
    }
}

#endif /* FEATURE_ID_REGISTRY_HPP */

