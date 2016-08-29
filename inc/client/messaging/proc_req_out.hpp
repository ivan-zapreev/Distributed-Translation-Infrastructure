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
                         * @param job_token the job token string
                         * @param priority the job priority
                         * @return a new instance of the processor request for the pre-processing task.
                         */
                        static inline proc_req_out * get_pre_proc_req(const string & job_token, const int32_t priority) {
                            return new proc_req_out(msg_type::MESSAGE_PRE_PROC_JOB_REQ, job_token, priority);
                        }

                        /**
                         * Allows to get a new instance of the processor request for the post-processing task.
                         * @param job_token the job token string
                         * @param priority the job priority
                         * @return a new instance of the processor request for the post-processing task.
                         */
                        static inline proc_req_out * get_post_proc_req(const string & job_token, const int32_t priority) {
                            return new proc_req_out(msg_type::MESSAGE_POST_PROC_JOB_REQ, job_token, priority);
                        }

                        /**
                         * Allows to set the text piece - the chunk of utf-8 characters
                         * @param chunk the text piece
                         * @param chunk_idx the text piece index
                         * @param num_chunks the number of text pieces
                         */
                        inline void set_chunk(const string & chunk, const uint64_t chunk_idx, const uint64_t num_chunks) {
                            m_writer.String(NUM_CHUNKS_FIELD_NAME);
                            m_writer.Int(num_chunks);
                            m_writer.String(CHUNK_IDX_FIELD_NAME);
                            m_writer.Int(chunk_idx);
                            m_writer.String(CHUNK_FIELD_NAME);
                            m_writer.String(chunk.c_str());
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
                         * Allows to set the job token
                         * @param job_token the job token
                         */
                        inline void set_job_uid(const string & job_token) {
                            m_writer.String(JOB_TOKEN_FIELD_NAME);
                            m_writer.String(job_token.c_str());
                        }
                        
                        /**
                         * Allows to get the job priority as specified by the client
                         * @return the job priority as specified by the client
                         */
                        inline void set_priority(const int32_t priority) {
                            m_writer.String(PRIORITY_NAME);
                            m_writer.Int(priority);
                        }

                    private:

                        /**
                         * This is the basic class constructor that accepts the
                         * source text md5 sum. It is made private because the first
                         * argument should not be set by the user.
                         * @param type the message type
                         * @param job_token the job token
                         * @param priority the job priority
                         */
                        proc_req_out(const msg_type type, const string & job_token, const int32_t priority)
                        : outgoing_msg(type), proc_req() {
                            set_job_uid(job_token);
                            set_priority(priority);
                        }
                    };
                }
            }
        }
    }
}

#endif /* PROC_REQ_OUT_HPP */

