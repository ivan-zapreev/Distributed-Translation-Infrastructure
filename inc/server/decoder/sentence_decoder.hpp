/* 
 * File:   sentence_translator.hpp
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
 * Created on February 11, 2016, 4:54 PM
 */

#ifndef SENTENCE_DECODER_HPP
#define SENTENCE_DECODER_HPP

#include "common/utils/threads.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/string_utils.hpp"

#include "server/decoder/de_parameters.hpp"

#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"
#include "server/lm/lm_configurator.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::rm;
using namespace uva::smt::bpbd::server::lm;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

                    /**
                     * This class represents a sentence translator utility.
                     * It receives a sentence to translate. Performs tokenization,
                     * lowercasing, splitting it into sub-phrases, performs decoding
                     * provides recombines the result into the target sentence.
                     */
                    class sentence_decoder {
                    public:

                        /**
                         * The basic constructor
                         * @param params the reference to the decoder parameters
                         */
                        sentence_decoder(const de_parameters & params)
                        : m_params(params),
                        m_lm_query(lm_configurator::allocate_query_proxy()),
                        m_tm_query(tm_configurator::allocate_query_proxy()),
                        m_rm_query(rm_configurator::allocate_query_proxy()) {
                            LOG_DEBUG << "Created a sentence decoder " << (string) m_params << END_LOG;
                        }

                        /**
                         * The basic destructor
                         */
                        ~sentence_decoder() {
                            //Dispose the query objects are they are no longer needed
                            lm_configurator::dispose_query_proxy(m_lm_query);
                            tm_configurator::dispose_query_proxy(m_tm_query);
                            rm_configurator::dispose_query_proxy(m_rm_query);
                        }

                        /**
                         * This is the main method needed to be called for translating a sentence.
                         * @param is_stop the flag that will be set to true in case 
                         *                one needs to abort the translation process.
                         * @param source_sent [in] the source language sentence to translate
                         *                         the source sentence is expected to be
                         *                         tokenized, reduced, and in the lower case.
                         * @param target_sent [out] the resulting target language sentence
                         */
                        inline void translate(acr_bool_flag is_stop,
                                string source_sent, string & target_sent) {
                            //If the reduced source sentence is not empty then do the translation
                            if (source_sent.size() != 0) {

                                //Prepare the source sentence for being decoded
                                set_source_sent(is_stop, source_sent);

                                //Return in case we need to stop translating
                                if (is_stop) return;

                                //Query the translation model
                                query_translation_model(is_stop);

                                //Return in case we need to stop translating
                                if (is_stop) return;

                                //Query the reordering model
                                query_reordering_model(is_stop);

                                //Return in case we need to stop translating
                                if (is_stop) return;

                                //Perform the translation
                                perform_translation(is_stop, source_sent, target_sent);
                            }
                        }

                    protected:

                        /**
                         * Allows to set the source sentence, this includes preparing things for decoding
                         * @param source_sent the source sentence to be set, accepts a reduced, tokenized,
                         *                    and in the lower case
                         */
                        inline void set_source_sent(acr_bool_flag is_stop, string & source_sent) {
                            //ToDo: Tokenize the sentence string
                        }

                        /**
                         * Allows to query the translation model based on the set sentence phrases
                         */
                        inline void query_translation_model(acr_bool_flag is_stop) {
                            //ToDo: Implement
                        }

                        /**
                         * Allows to query the reordering model based on the set sentence phrases
                         */
                        inline void query_reordering_model(acr_bool_flag is_stop) {
                            //ToDo: Implement
                        }

                        /**
                         * Performs the sentence translation 
                         */
                        inline void perform_translation(acr_bool_flag is_stop, const string source_sent, string & target_sent) {
                            //ToDo: Implement

                            const uint32_t time_sec = rand() % 20;
                            for (uint32_t i = 0; i <= time_sec; ++i) {
                                if (is_stop) break;
                                this_thread::sleep_for(chrono::seconds(1));
                            }
                            
                            //ToDo: Remove this temporary plug
                            target_sent = source_sent;
                        }

                    private:
                        //Stores the reference to the decoder parameters
                        const de_parameters & m_params;
                        //The language mode query proxy
                        lm_query_proxy & m_lm_query;
                        //The language mode query proxy
                        tm_query_proxy & m_tm_query;
                        //The language mode query proxy
                        rm_query_proxy & m_rm_query;
                    };
                }
            }
        }
    }
}

#endif /* SENTENCE_TRANSLATOR_HPP */

