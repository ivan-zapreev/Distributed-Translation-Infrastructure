/* 
 * File:   trans_info.hpp
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
 * Created on May 2, 2016, 11:34 AM
 */

#ifndef TRANS_INFO_PROVIDER_HPP
#define TRANS_INFO_PROVIDER_HPP

#include <map>
#include <string>
#include <iomanip>

#include "common/utils/logging/logger.hpp"

#include "server/server_configs.hpp"
#include "server/messaging/trans_sent_data_out.hpp"

using namespace std;

using namespace uva::utils::logging;

using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * The translation info provider class interface
                 */
                class trans_info_provider {
                public:

                    /**
                     * The basic virtual destructor
                     */
                    virtual ~trans_info_provider() {
                    }

                    /**
                     * The method that is to be implemented in for getting the translation info
                     * @param sent_data [in/out] the sentence data object to be filled in with the translation info
                     */
                    virtual void get_trans_info(trans_sent_data_out & sent_data) const = 0;

#if IS_SERVER_TUNING_MODE
                    /**
                     * Is needed to dump the search lattice data for the given sentence.
                     * This method is to be called after a translation is successfully finished.
                     * @param lattice_dump the stream the lattice is to be dumped into.
                     * @param scores_dump the stream the scores are to be dumped into.
                     */
                    virtual void dump_search_lattice(ostream & lattice_dump, ostream & scores_dump) const = 0;
#endif
                };
            }
        }
    }
}

#endif /* TRANS_INFO_HPP */

