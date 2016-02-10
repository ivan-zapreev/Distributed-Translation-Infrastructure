/* 
 * File:   rm_basic_builder.hpp
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
 * Created on February 8, 2016, 9:56 AM
 */

#ifndef RM_BASIC_BUILDER_HPP
#define RM_BASIC_BUILDER_HPP

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace rm {
                    namespace builders {

                        //Stores the translation model delimiter character for parsing one line
#define TM_DELIMITER '|'
                        //Stores the translation model delimiter character cardinality
#define TM_DELIMITER_CDTY 3

                        /**
                         * This class represents a basic reader of the reordering model.
                         * It allows to read a text-formatted reordering model and to put
                         * it into the given instance of the model class. It assumes the
                         * simple text model format as used by Oyster or Moses.
                         * See http://www.statmt.org/moses/?n=Moses.Tutorial for some info.
                         * The reordering model is also commonly known as a phrase table.
                         */
                        template< typename model_type, typename reader_type>
                        class rm_basic_builder {
                        public:

                            /**
                             * The basic constructor of the builder object
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            rm_basic_builder(model_type & model, reader_type & reader)
                            : m_model(model), m_reader(reader) {
                            }
                            
                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
                             * of distinct source phrases.
                             */
                            void build(){
                                //Count and set the number of source phrases if needed
                                if (m_model.is_num_entries_needed()) {
                                    set_number_source_phrases();
                                }

                                //Process the translations
                                process_source_entries();
                            }
                            
                        protected:

                            /**
                             * Allows to count and set the number of source phrases
                             */
                            inline void set_number_source_phrases() {
                                //ToDo: Implement
                            }

                            /**
                             * Allows to process translations.
                             */
                            void process_source_entries() {
                                //ToDo: Implement
                            }
                            
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

#endif /* RM_BASIC_BUILDER_HPP */

