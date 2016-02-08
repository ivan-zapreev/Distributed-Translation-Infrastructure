/* 
 * File:   tm_builder.hpp
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
 * Created on February 8, 2016, 9:55 AM
 */

#ifndef TM_BUILDER_HPP
#define TM_BUILDER_HPP

#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::file;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace tm {
                    namespace builders {

                        //Stores the translation model delimiter character for parsing one line
#define TM_DELIMITER '|'
                        //Stores the translation model delimiter character cardinality
#define TM_DELIMITER_CDTY 3
                        
                        /**
                         * This class represents a basic reader of the translation model.
                         * It allows to read a text-formatted translation model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The translation model is also commonly known as a phrase table.
                         */
                        template< typename model_type, typename reader_type>
                        class tm_basic_builder {
                        public:

                            /**
                             * The basic constructor of the builder object
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            tm_basic_builder(model_type & model, reader_type & reader)
                            : m_model(model), m_reader(reader) {
                            }

                            /**
                             * Allows to build the model by reading the from reader
                             */
                            void build() {
                                //Declare the text piece readers for storing the model file line and its parts
                                TextPieceReader line, source, target, weights;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the from phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);
                                    //Read the to phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);
                                    //Read the the probability weights
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(weights);
                                    
                                    LOG_USAGE << source << "|||" << target << "|||" << weights << END_LOG;
                                    
                                    //ToDo: Implement
                                }
                            }
                        protected:

                        private:
                            //Stores the reference to the model
                            model_type & m_model;
                            //Stores the reference to the builder;
                            reader_type & m_reader;
                        };
                    }
                }
            }
        }
    }
}

#endif /* TM_BUILDER_HPP */

