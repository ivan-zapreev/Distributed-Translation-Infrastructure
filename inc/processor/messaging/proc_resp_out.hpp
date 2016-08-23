/* 
 * File:   proc_resp_out.hpp
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
 * Created on July 28, 2016, 2:56 PM
 */

#ifndef PROC_RESP_OUT_HPP
#define PROC_RESP_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/proc_resp.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace processor {
                namespace messaging {

                    /**
                     * This class represents a processor job response message.
                     */
                    class proc_resp_out : public outgoing_msg, public proc_resp {
                    public:

                        /**
                         * Allows to get a new instance of the processor response for the pre-processing task.
                         * @param job_token the job token
                         * @param status_code the job's result code
                         * @param status_msg the job's status message
                         * @return a new instance of the processor response for the pre-processing task.
                         */
                        static inline proc_resp_out * get_pre_proc_resp(const string & job_token,
                                const status_code code, const string & msg) {
                            return new proc_resp_out(msg_type::MESSAGE_PRE_PROC_JOB_RESP, job_token, code, msg);
                        }

                        /**
                         * Allows to get a new instance of the processor response for the post-processing task.
                         * @param job_token the job token
                         * @param status_code the job's result code
                         * @param status_msg the job's status message
                         * @return a new instance of the processor response for the post-processing task.
                         */
                        static inline proc_resp_out * get_post_proc_resp(const string & job_token,
                        const status_code code, const string & msg) {
                            return new proc_resp_out(msg_type::MESSAGE_POST_PROC_JOB_RESP, job_token, code, msg);
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
                         * Allows to set the message status: code and message
                         * @param code the status code
                         * @param msg the status message
                         */
                        inline void set_status(const status_code & code, const string & msg) {
                            m_writer.String(STAT_CODE_FIELD_NAME);
                            m_writer.Int(code.val());
                            m_writer.String(STAT_MSG_FIELD_NAME);
                            m_writer.String(msg.c_str());
                        }

                        /**
                         * Allows to get the job token
                         * @param job_token the job token
                         */
                        inline void set_job_token(const string & job_token) {
                            m_writer.String(JOB_TOKEN_FIELD_NAME);
                            m_writer.String(job_token.c_str());
                        }

                    private:

                        /**
                         * This is the basic class constructor that accepts the
                         * source text md5 sum, the result code and the status message.
                         * It is made private because the first argument should
                         * not be set by the user.
                         * @param type the message type
                         * @param job_token the job token
                         * @param status_code the job's result code
                         * @param status_msg the job's status message
                         */
                        proc_resp_out(const msg_type type, const string & job_token,
                                const status_code code, const string & msg)
                        : outgoing_msg(type), proc_resp() {
                            //Set the values using the setter methods
                            set_job_token(job_token);
                            set_status(code, msg);
                        }
                    };
                }
            }
        }
    }
}

#endif /* PROC_RESP_OUT_HPP */

