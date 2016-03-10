/* 
 * File:   tm_configs.hpp
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
 * Created on February 9, 2016, 11:53 AM
 */

#ifndef TM_CONFIGS_HPP
#define TM_CONFIGS_HPP

#include "server/server_configs.hpp"

#include "common/utils/file/cstyle_file_reader.hpp"

#include "server/tm/models/tm_basic_model.hpp"
#include "server/tm/builders/tm_basic_builder.hpp"

using namespace std;

using namespace uva::utils::file;

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::builders;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace tm {
                    //Define the default model type to be used
                    typedef tm_basic_model tm_model_type;

                    //Define the builder type 
                    typedef tm_basic_builder<tm_model_type, cstyle_file_reader> tm_builder_type;
                }
            }
        }
    }
}

#endif /* TM_CONFIGS_HPP */

