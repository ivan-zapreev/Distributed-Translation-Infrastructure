/* 
 * File:   lm_configs.hpp
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
 * Created on September 18, 2015, 8:54 AM
 */

#ifndef LM_CONFIGS_HPP
#define LM_CONFIGS_HPP

#include <inttypes.h>
#include <string>

#include "server/server_configs.hpp"

#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/lm/dictionaries/basic_word_index.hpp"
#include "server/lm/dictionaries/counting_word_index.hpp"
#include "server/lm/dictionaries/optimizing_word_index.hpp"
#include "server/lm/dictionaries/hashing_word_index.hpp"

#include "server/lm/builders/lm_basic_builder.hpp"

#include "server/lm/models/c2d_hybrid_trie.hpp"
#include "server/lm/models/c2d_map_trie.hpp"
#include "server/lm/models/c2w_array_trie.hpp"
#include "server/lm/models/g2d_map_trie.hpp"
#include "server/lm/models/h2d_map_trie.hpp"
#include "server/lm/models/w2c_array_trie.hpp"
#include "server/lm/models/w2c_hybrid_trie.hpp"

#include "server/lm/builders/lm_basic_builder.hpp"

using namespace std;

using namespace uva::utils::file;

using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::arpa;
using namespace uva::smt::bpbd::server::lm::dictionary;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    //Here we have a default word index, see the lm_confgs for the recommended word index information!
                    typedef hashing_word_index lm_word_index;

                    //Here we have a default trie type
                    typedef H2DMapTrie<lm_word_index> lm_model_type;

                    //Define the builder type 
                    typedef lm_basic_builder<lm_model_type, CStyleFileReader> lm_builder_type;
                }
            }
        }
    }
}


#endif /* CONFIGURATION_HPP */

