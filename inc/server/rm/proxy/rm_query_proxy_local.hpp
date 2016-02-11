/* 
 * File:   rm_query_proxy_local.hpp
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
 * Created on February 8, 2016, 10:00 AM
 */

#ifndef RM_QUERY_PROXY_LOCAL_HPP
#define RM_QUERY_PROXY_LOCAL_HPP

#include "server/rm/proxy/rm_query_proxy.hpp"
#include "server/rm/models/rm_query.hpp"

using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::rm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace proxy {

                        /**
                         * This is a local implementation of the reordering model query
                         * This implementation works with the local reordering model
                         */
                        template<typename model_type>
                        class rm_query_proxy_local : public rm_query_proxy<model_type::NUM_WEIGHTS> {
                        public:
                            //Make the base typedef
                            typedef rm_query_proxy<model_type::NUM_WEIGHTS> BASE;
                            
                            //Make a local typedef for the rm entry
                            typedef typename BASE::rm_num_entry rm_num_entry;

                            /**
                             * The basic constructor that accepts the reordering model reference to query to
                             * @param model the reordering model to query
                             */
                            rm_query_proxy_local(const model_type & model) : m_query(model) {
                            }
  
                            /**
                             * @see rm_query_proxy
                             */
                            virtual void set_st_uids(const vector<phrase_uid> * const uids) {
                                m_query.set_st_uids(uids);
                            }

                            /**
                             * @see rm_query_proxy
                             */
                            virtual const rm_num_entry & get_reordering(const phrase_uid uid) {
                                return m_query.get_reordering(uid);
                            }
                            
                            /**
                             * @see rm_query_proxy
                             */
                            virtual void execute() {
                                m_query.execute();
                            }
                            
                            /**
                             * @see rm_query_proxy
                             */
                            virtual ~rm_query_proxy_local(){
                                //Nothing to be done, no dynamically allocated resources
                            }
                            
                        private:
                            //Stores the actual query
                            rm_query<model_type> m_query;
                        };
                    }
                }
            }
        }
    }
}

#endif /* RM_QUERY_PROXY_LOCAL_HPP */

