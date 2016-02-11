/* 
 * File:   de_configurator.hpp
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
 * Created on February 11, 2016, 5:17 PM
 */

#ifndef DE_CONFIGURATOR_HPP
#define DE_CONFIGURATOR_HPP

#include "server/decoder/de_parameters.hpp"
#include "server/decoder/sentence_decoder.hpp"

using namespace uva::smt::bpbd::server::decoder;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace decoder {

                    /**
                     * This class represents a singleton that allows to
                     * configure the decoding server that can create
                     * decoder instances. The interface is implemented as
                     * the configurations to the translation, reordering,
                     * and language models
                     */
                    class de_configurator {
                    public:
                        /**
                         * This method allows to "connect" to the decoder.
                         * The latter means configure it using the given data.
                         * @param params the decoder parameters to be used.
                         */
                        static void connect(const de_parameters & params) {
                            //ToDo: Implement
                        }

                        /**
                         * Allows to disconnect from the decoder, i.e. clean up the memory etc.
                         */
                        static void disconnect() {
                            //ToDo: Implement
                        }
 
                        /**
                         * Allows to get an instance of the decoder object.
                         * @return an instance of the decoder object.
                         */
                        static inline sentence_decoder & allocate_decoder() {
                            //ToDo: Pre-allocate decoders, make as many as there are threads
                            return *(new sentence_decoder());
                        }
                        
                        /**
                         * Allows to dispose the decoder
                         * @param dec the decoder to be returned
                         */
                        static inline void dispose_decoder(sentence_decoder & dec) {
                            //ToDo: Mark the decoder instance as available
                            delete &dec;
                        }
                        
                    protected:
                    private:
                    };
                }
            }
        }
    }
}

#endif /* DE_CONFIGURATOR_HPP */

