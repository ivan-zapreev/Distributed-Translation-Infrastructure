/* 
 * File:   trans_job_req_out.hpp
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
 * Created on June 23, 2016, 3:38 PM
 */

#ifndef TRANS_JOB_REQ_OUT_HPP
#define TRANS_JOB_REQ_OUT_HPP

#include "common/messaging/outgoing_msg.hpp"
#include "common/messaging/trans_job_req.hpp"

using namespace uva::smt::bpbd::common::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace client {
                namespace messaging {

                    /**
                     * This class represents an outgoing translation
                     * job request, created on the client side.
                     */
                    class trans_job_req_out : public outgoing_msg, public trans_job_req {
                    public:

                        /**
                         * This is the basic class constructor that accepts the
                         * translation job id, the translation text and source
                         * and target language strings.
                         * @param job_id the translation job id
                         * @param source_lang the source language string
                         * @param source_text the text in the source language to translate
                         * @param target_lang the target language string
                         * @param is_trans_info true if the client should requests the translation info from the server
                         */
                        trans_job_req_out(const job_id_type job_id, const string & source_lang,
                                vector<string> & source_text, const string & target_lang, const bool is_trans_info)
                        : outgoing_msg(), trans_job_req(msg_type::MESSAGE_TRANS_JOB_REQ) {
                            get_json()[JOB_ID_FIELD_NAME] = job_id;
                            get_json()[SOURCE_LANG_FIELD_NAME] = source_lang;
                            get_json()[TARGET_LANG_FIELD_NAME] = target_lang;
                            get_json()[IS_TRANS_INFO_FIELD_NAME] = is_trans_info;
                            get_json()[SOURCE_SENTENCES_FIELD_NAME] = source_text;

                            LOG_DEBUG << "Translation job request, job id: " << job_id
                                    << " source language: " << source_lang
                                    << " target language: " << target_lang
                                    << " translation info flag: " << is_trans_info << END_LOG;
                        }

                        /**
                         * The basic destructor
                         */
                        virtual ~trans_job_req_out() {
                            //Nothing to be done here
                        }
                    };

                }
            }
        }
    }
}

#endif /* TRANS_JOB_REQ_OUT_HPP */

