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

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/string_utils.hpp"

#include "server/tm/models/tm_phrase_id.hpp"
#include "server/tm/models/tm_target_entry.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::translation::server::tm::models;

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
                             * Allows to build the model by reading the from reader.
                             * This is a two pass process as first we need the number
                             * of distinct source phrases.
                             */
                            void build() {
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
                                //Declare the sizes map
                                sizes_map * sizes = new sizes_map();

                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source;
                                //Stores the number of source entries for logging
                                size_t num_source = 0;

                                //Compute the source entry sizes
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);
                                    //Get the current source phrase uid
                                    string source_str = source.str();
                                    phrase_uid uid = get_phrase_uid(trim(source_str));
                                    //Increment the count for the given source uid
                                    ++sizes->operator[](uid);

                                    if (sizes->operator[](uid) == 1) {
                                        ++num_source;
                                        LOG_DEBUG << num_source << ") Source: " << source_str << " uid: " << uid << END_LOG;
                                    }

                                    LOG_DEBUG1 << "-> translation count: " << sizes->at(uid) << END_LOG;
                                }

                                //Set the number of entries into the model
                                m_model.set_num_entries(sizes);

                                //Re-set the reader to start all over again
                                m_reader.reset();
                            }

                            /**
                             * The line format assumes source to target and then at least four weights as given by:
                             *     http://www.statmt.org/moses/?n=FactoredTraining.ScorePhrases
                             * Currently, four different phrase translation scores are computed:
                             *    inverse phrase translation probability φ(f|e)
                             *    inverse lexical weighting lex(f|e)
                             *    direct phrase translation probability φ(e|f)
                             *    direct lexical weighting lex(e|f) 
                             * Previously, there was another score:
                             *    phrase penalty (always exp(1) = 2.718) 
                             * The latter is considered optional, all the other elements
                             * followed on the translation line are now skipped.
                             * @param source_entry the pointer to the source entry for which this translation is
                             * @param rest stores the line to be parsed into a translation entry
                             */
                            inline void process_target_entries(tm_source_entry * source_entry, TextPieceReader &rest) {
                                LOG_DEBUG2 << "Got translation line to parse: " << rest << END_LOG;

                                TextPieceReader target;

                                //Read the target phrase
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                LOG_DEBUG2 << "The target phrase is: " << target << END_LOG;
                                
                                //Add the translation entry to the model
                                string target_str = target.str();
                                tm_target_entry & target_entry = source_entry->new_translation(trim(target_str));

                                LOG_DEBUG1 << "-> Translation: " << target_str << END_LOG;

                                //Set the target weights
                                TextPieceReader token;

                                //Skip the first space
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not skip the first space!");

                                //Read the second weight 
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not read the inverse phrase translation probability φ(f|e)!");
                                fast_s_to_f(target_entry.get_sct_prob(), token.str().c_str());

                                //Read the third weight 
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not read the inverse lexical weighting lex(f|e)!");
                                fast_s_to_f(target_entry.get_sct_lex(), token.str().c_str());

                                //Read the fourth weight 
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not read the direct phrase translation probability φ(e|f)!");
                                fast_s_to_f(target_entry.get_tcs_prob(), token.str().c_str());

                                //Read the fourth weight 
                                ASSERT_CONDITION_THROW(!rest.get_first_space(token), "Could not read the direct lexical weighting lex(e|f)!");
                                fast_s_to_f(target_entry.get_tcs_lex(), token.str().c_str());
                            }

                            /**
                             * Allows to process translations.
                             */
                            void process_source_entries() {
                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source;
                                //Store the current source and next source uids
                                phrase_uid curr_source_uid = UNDEFINED_PHRASE_ID;
                                phrase_uid next_source_uid = UNDEFINED_PHRASE_ID;
                                //Store the source entry
                                tm_source_entry * source_entry = NULL;
                                //Stores the number of source entries for logging
                                size_t num_source = 0;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uid
                                    string source_str = source.str();
                                    next_source_uid = get_phrase_uid(trim(source_str));

                                    //In case we have a new source, then add a source entry
                                    if (curr_source_uid != next_source_uid) {
                                        //Finalize the previous entry if there was one
                                        if (curr_source_uid != UNDEFINED_PHRASE_ID) {
                                            m_model.finalize_entry(curr_source_uid);
                                        }
                                        //Increment the number of source entries
                                        ++num_source;

                                        //Store the new source id
                                        curr_source_uid = next_source_uid;

                                        //Open the new entry
                                        source_entry = m_model.begin_entry(curr_source_uid);
                                    }

                                    LOG_DEBUG << num_source << ") Source: " << source_str << " uid: " << curr_source_uid << END_LOG;

                                    //Parse the rest of the target entry
                                    process_target_entries(source_entry, line);
                                }

                                //Finalize the previous entry if there was one
                                if (curr_source_uid != UNDEFINED_PHRASE_ID) {
                                    m_model.finalize_entry(curr_source_uid);
                                }

                                //finalize the model
                                m_model.finalize();
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

#endif /* TM_BUILDER_HPP */

