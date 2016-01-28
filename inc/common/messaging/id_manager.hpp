/* 
 * File:   id_manager.hpp
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
 * Created on January 19, 2016, 2:33 PM
 */

#ifndef SYNCHRONIZED_ID_MANAGER_HPP
#define SYNCHRONIZED_ID_MANAGER_HPP

#include <websocketpp/common/thread.hpp>

namespace uva {
    namespace smt {
        namespace decoding {
            namespace common {
                namespace messaging {

                    /**
                     * This class is synchronized and an instance of the class
                     * is to be used in case one needs continuous ids to be
                     * issued in a multi-threaded environment.
                     */
                    template<typename id_type>
                    class id_manager {
                    public:
                        typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;

                        /**
                         * The basic class constructor for the id issuing entity.
                         * It is recommended to have the minimum value for the id
                         * higher that that allowed by the type itself. This way
                         * one can check for overflows.
                         * @param min_id the minimum value of id
                         */
                        id_manager(const id_type min_id) : m_min_id(min_id), m_next_id(min_id) {
                        }

                        /**
                         * Allows to get the next id. This method is thread safe
                         * due to mutex locking.
                         * @return the next id
                         */
                        inline uint32_t get_next_id() {
                            scoped_lock guard(m_lock_id);

                            return m_next_id++;
                        }

                        /**
                         * Allows to get the minimum value of the issued ids
                         * @return the minimum value of the issued ids.
                         */
                        const id_type & get_min_id() const {
                            return m_min_id;
                        }

                    private:
                        //The stored minimum value of the id
                        const id_type m_min_id;
                        //Stores the next id to be used
                        uint32_t m_next_id;
                        //Stores the synchronization mutex for issuing new ids
                        websocketpp::lib::mutex m_lock_id;

                    };
                }
            }
        }
    }
}


#endif /* SYNCHRONIZED_ID_MANAGER_HPP */

