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

#include "server/tm/tm_configurator.hpp"
#include "server/rm/rm_configurator.hpp"
#include "server/lm/lm_configurator.hpp"

using namespace std;

using namespace uva::utils::threads;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

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
                         * This is the main method needed to be called for translating a sentence.
                         * @param is_stop the flag that will be set to true in case 
                         *                one needs to abort the translation process.
                         * @param source_sentence [in] the source language sentence to translate
                         * @param target_sentence [out] the resulting target language sentence
                         */
                        inline void translate(const atomic<bool> & is_stop,
                                const string & source_sentence,
                                string & target_sentence) {
                            
                            //Obtain the language mode query proxy
                            //lm_query_proxy *  lm_query = lm_configurator::get_query_proxy();
                            
                            //Obtain the language mode query proxy
                            //tm_query_proxy *  tm_query = tm_configurator::get_query_proxy();
                            
                            //Obtain the language mode query proxy
                            //rm_query_proxy *  rm_query = rm_configurator::get_query_proxy();
                            
                            //ToDo: Implement, implement the translation process, the next loop
                            //is a temporary measure for testing, do the actual decoding steps
                            if (source_sentence.size() != 0) {
                                const uint32_t time_sec = rand() % source_sentence.size();
                                for (uint32_t i = 0; i <= time_sec; ++i) {
                                    if (is_stop) break;
                                    this_thread::sleep_for(chrono::seconds(1));
                                }
                            }
                        }

                    protected:
                    private:
                    };
                }
            }
        }
    }
}

#endif /* SENTENCE_TRANSLATOR_HPP */

