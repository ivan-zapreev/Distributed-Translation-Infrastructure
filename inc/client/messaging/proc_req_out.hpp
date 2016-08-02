/* 
 * File:   proc_req_out.hpp
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
 * Created on August 2, 2016, 9:19 AM
 */

#ifndef PROC_REQ_OUT_HPP
#define PROC_REQ_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/proc_req.hpp"
#include "common/messaging/job_id.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents a processor job request message.
                     */
                    class proc_req_out : public outgoing_msg, public proc_req {
                    public:

                        /**
                         * Allows to get a new instance of the processor request for the pre-processing task.
                         * @param job_id the client-issued id of the job 
                         * @return a new instance of the processor request for the pre-processing task.
                         */
                        static inline proc_req_out * get_pre_proc_resp(const job_id_type job_id) {
                            return new proc_req_out(msg_type::MESSAGE_PRE_PROC_JOB_REQ, job_id);
                        }

                        /**
                         * Allows to get a new instance of the processor request for the post-processing task.
                         * @param job_id the client-issued id of the job 
                         * @return a new instance of the processor request for the post-processing task.
                         */
                        static inline proc_req_out * get_post_proc_resp(const job_id_type job_id) {
                            return new proc_req_out(msg_type::MESSAGE_POST_PROC_JOB_REQ, job_id);
                        }

                        /**
                         * Allows to set the text piece - the chunk of utf-8 characters
                         * @param chunk the text piece
                         * @param chunk_idx the text piece index
                         * @param num_chunks the number of text pieces
                         */
                        inline void set_chunk(const string & chunk, const uint64_t chunk_idx, const uint64_t num_chunks) {
                            m_writer.String(CHUNK_IDX_FIELD_NAME);
                            m_writer.String(chunk.c_str());
                            m_writer.String(CHUNK_FIELD_NAME);
                            m_writer.Int(chunk_idx);
                            m_writer.String(NUM_CHUNKS_FIELD_NAME);
                            m_writer.Int(num_chunks);
                        }
                        
                        /**
                         * Allows to set the language of the text piece
                         * @param lang the language of the text piece
                         */
                        inline void set_language(const string & lang) {
                            m_writer.String(LANG_FIELD_NAME);
                            m_writer.String(lang.c_str());
                        }
                        
                    protected:

                        /**
                         * Allows to get the client-issued job id
                         * @return the client-issued job id
                         */
                        inline void set_job_id(const job_id_type job_id) {
                            m_writer.String(JOB_ID_FIELD_NAME);
                            m_writer.Uint64(job_id);
                        }
                        
                    private:

                        /**
                         * This is the basic class constructor that accepts the
                         * processor job id. It is made private because the first
                         * argument should not be set by the user.
                         * @param type the message type
                         * @param job_id the client-issued id of the job 
                         */
                        proc_req_out(const msg_type type, const job_id_type job_id)
                        : outgoing_msg(type), proc_req() {
                            //Set the values using the setter methods
                            set_job_id(job_id);
                        }
                    };
                }
            }
        }
    }
}

#endif /* PROC_REQ_OUT_HPP */

