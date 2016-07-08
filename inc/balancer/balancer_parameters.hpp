/* 
 * File:   balancer_parameters.hpp
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
 * Created on July 8, 2016, 10:36 AM
 */

#ifndef BALANCER_PARAMETERS_HPP
#define BALANCER_PARAMETERS_HPP

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the storage for balancer parameters:
                 * Responsibilities:
                 *      Store the run-time parameters of the balancer application
                 */
                class balancer_parameters {
                public:

                    /**
                     * The basic constructor
                     */
                    balancer_parameters() {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~balancer_parameters() {
                    }

                    /**
                     * Allows to finalize the parameters after loading.
                     */
                    void finalize() {
                        //ToDo: Implement
                        THROW_NOT_IMPLEMENTED();
                    }

                private:
                };
            }
        }
    }
}

#endif /* BALANCER_PARAMETERS_HPP */

