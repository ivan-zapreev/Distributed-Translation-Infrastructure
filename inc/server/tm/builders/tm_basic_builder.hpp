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

#include "server/common/models/phrase_uid.hpp"
#include "server/tm/tm_parameters.hpp"
#include "server/tm/models/tm_target_entry.hpp"
#include "server/tm/models/tm_source_entry.hpp"

using namespace std;

using namespace uva::utils::exceptions;
using namespace uva::utils::logging;
using namespace uva::utils::file;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::server::common::models;
using namespace uva::smt::bpbd::server::tm;
using namespace uva::smt::bpbd::server::tm::models;

namespace uva {
    namespace smt {
        namespace bpbd {
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
                             * @params params the model parameters
                             * @param model the model to put the data into
                             * @param reader the reader to read the data from
                             */
                            tm_basic_builder(const tm_parameters & params, model_type & model, reader_type & reader)
                            : m_params(params), m_model(model), m_reader(reader) {
                            }

                            /**
                             * Allows to build the model by reading from the reader object.
                             * This is a two step process as first we need the number
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
                                Logger::start_progress_bar(string("Counting phrase translations"));

                                //Declare the sizes map
                                sizes_map * sizes = new sizes_map();

                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source;
                                //Stores the number of source entries for logging
                                size_t num_source = 0;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;

                                //Compute the source entry sizes
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uids
                                    string next_source_str = source.str();
                                    trim(next_source_str);
                                    if (source_str != next_source_str) {
                                        //Store the new source string
                                        source_str = next_source_str;
                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);
                                    }

                                    //Increment the count for the given source uid
                                    ++sizes->operator[](source_uid);

                                    if (sizes->operator[](source_uid) == 1) {
                                        ++num_source;
                                        LOG_DEBUG << num_source << ") Source: " << source_str << " uid: " << source_uid << END_LOG;
                                    }

                                    //Update the progress bar status
                                    Logger::update_progress_bar();

                                    LOG_DEBUG1 << "-> translation count: " << sizes->at(source_uid) << END_LOG;
                                }

                                //Set the number of entries into the model
                                m_model.set_num_entries(sizes);

                                //Re-set the reader to start all over again
                                m_reader.reset();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();
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
                                LOG_DEBUG2 << "Got translation line to parse: ___" << rest << "___" << END_LOG;

                                //Declare the target entry storing reader
                                TextPieceReader target;

                                //Read the target phrase, it is surrounded by spaces
                                rest.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(target);

                                LOG_DEBUG2 << "The target phrase is: " << target << END_LOG;

                                //Add the translation entry to the model
                                string target_str = target.str();
                                trim(target_str);
                                //Compute the target phrase and its uid
                                const phrase_uid target_uid = get_phrase_uid<true>(target_str);
                                tm_target_entry & target_entry = source_entry->new_translation(target_str, target_uid);

                                LOG_DEBUG1 << "-> Translation: ___" << target_str << "___" << END_LOG;

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
                                Logger::start_progress_bar(string("Building translation model"));

                                //Declare the text piece reader for storing the read line and source phrase
                                TextPieceReader line, source;

                                //Store the source entry
                                tm_source_entry * source_entry = NULL;
                                //Stores the number of source entries for logging
                                size_t num_source = 0;

                                //Store the cached source string and its uid values
                                string source_str = "";
                                phrase_uid source_uid = UNDEFINED_PHRASE_ID;

                                //Start reading the translation model file line by line
                                while (m_reader.get_first_line(line)) {
                                    //Read the source phrase
                                    line.get_first<TM_DELIMITER, TM_DELIMITER_CDTY>(source);

                                    //Get the current source phrase uid
                                    string next_source_str = source.str();
                                    trim(next_source_str);
                                    if (source_str != next_source_str) {
                                        //Store the new source string
                                        source_str = next_source_str;

                                        //Finalize the previous entry if there was one
                                        if (source_uid != UNDEFINED_PHRASE_ID) {
                                            m_model.finalize_entry(source_uid);
                                        }
                                        
                                        //Increment the number of source entries
                                        ++num_source;

                                        //Compute the new source string uid
                                        source_uid = get_phrase_uid(source_str);

                                        //Open the new source entry
                                        source_entry = m_model.begin_entry(source_uid);
                                    }

                                    LOG_DEBUG << num_source << ") Source: " << source_str << " uid: " << source_uid << END_LOG;

                                    //Parse the rest of the target entry
                                    process_target_entries(source_entry, line);

                                    //Update the progress bar status
                                    Logger::update_progress_bar();
                                }

                                //Finalize the previous entry if there was one
                                if (source_uid != UNDEFINED_PHRASE_ID) {
                                    m_model.finalize_entry(source_uid);
                                }

                                //Finalize the model
                                m_model.finalize();

                                //Stop the progress bar in case of no exception
                                Logger::stop_progress_bar();
                            }

                        private:
                            //Stores the reference to the model parameters
                            const tm_parameters & m_params;
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

