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

#ifndef TRANS_INFO_HPP
#define	TRANS_INFO_HPP

#include <map>
#include <string>
#include <iomanip>

#include "common/utils/logging/logger.hpp"

using namespace std;

using namespace uva::utils::logging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {

                /**
                 * The forward declaration of the translation info class
                 */
                class trans_info;

                /**
                 * The translation info provider class interface
                 */
                class trans_info_provider {
                public:
                    /**
                     * The basic virtual destructor
                     */
                    virtual ~trans_info_provider() {}

                    /**
                     * The method that is to be implemented in for getting the translation info
                     * @param info
                     */
                    virtual void get_trans_info(trans_info & info) = 0;
                };

                /**
                 * Stores the translation process information for a single translation task.
                 * This information might be requested by the client to get more insight
                 * into the translation process.
                 */
                class trans_info {
                public:
                    typedef vector<uint32_t> stack_loads;
                    typedef stack_loads::const_iterator stack_loads_iter;

                    /**
                     * The basic constructor
                     */
                    trans_info() {
                    }

                    /**
                     * The basic destructor
                     */
                    virtual ~trans_info() {
                    }

                    /**
                     * Allows to clean the translation task info
                     */
                    void clean() {
                        //Clear the stack usage data
                        stack_data.clear();
                    }

                    /**
                     * Allows to add the next stack level usage statistics data.
                     * Note that the stack usage is added incrementally it is assumed
                     * that each next stack usage pushed is for the next stack elevel
                     * @param usage the percentage value of the stack usage, i.e.
                     * % of stack elements used.
                     */
                    void push_stack_usage(const float & usage) {
                        stack_data.push_back(usage);
                    }

                    /**
                     * Allows to serialize the info class into a single string
                     * @return the string representation of the class
                     */
                    string serialize() const {
                        ostringstream out;
                        out << "<info>: stack[ ";
                        for (stack_loads_iter iter = stack_data.begin(); iter != stack_data.end(); ++iter) {
                            out << *iter << "% ";
                        }
                        out << "]";
                        return out.str();
                    }

                private:
                    stack_loads stack_data;
                };
            }
        }
    }
}

#endif	/* TRANS_INFO_HPP */

