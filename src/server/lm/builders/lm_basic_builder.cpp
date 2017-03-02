/* 
 * File:   lm_basic_builder.cpp
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
 * Created on April 18, 2015, 11:58 AM
 */
#include <iostream>
#include <string>

#include "common/utils/logging/logger.hpp"
#include "common/utils/text/string_utils.hpp"
#include "common/utils/file/cstyle_file_reader.hpp"
#include "common/utils/file/file_stream_reader.hpp"
#include "common/utils/file/memory_mapped_file_reader.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/builders/lm_basic_builder.hpp"
#include "server/lm/builders/lm_gram_builder_factory.hpp"

#include "server/lm/models/c2d_map_trie.hpp"
#include "server/lm/models/w2c_hybrid_trie.hpp"
#include "server/lm/models/c2w_array_trie.hpp"
#include "server/lm/models/w2c_array_trie.hpp"
#include "server/lm/models/c2d_hybrid_trie.hpp"
#include "server/lm/models/g2d_map_trie.hpp"
#include "server/lm/models/h2d_map_trie.hpp"

using namespace uva::smt::bpbd::server::lm;
using namespace uva::utils::logging;
using namespace uva::utils::text;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace arpa {

                        template<typename trie_type, typename reader_type>
                        lm_basic_builder<trie_type, reader_type>::lm_basic_builder(const lm_parameters & params, trie_type & trie, reader_type & file)
                        : m_params(params), m_trie(trie), m_file(file), m_line(), m_ng_amount_reg_exp("ngram [[:d:]]+=[[:d:]]+") {
                        }

                        template<typename trie_type, typename reader_type>
                        lm_basic_builder<trie_type, reader_type>::lm_basic_builder(const lm_basic_builder<trie_type, reader_type>& orig)
                        : m_params(orig.m_params), m_trie(orig.m_trie), m_file(orig.m_file), m_line(orig.m_line), m_ng_amount_reg_exp("ngram [[:d:]]+=[[:d:]]+") {
                        }

                        template<typename trie_type, typename reader_type>
                        lm_basic_builder<trie_type, reader_type>::~lm_basic_builder() {
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::read_headers() {
                            LOG_DEBUG << "Start reading ARPA headers." << END_LOG;

                            while (true) {
                                LOG_DEBUG1 << "Read header (?) line: '" << m_line.str() << "'" << END_LOG;

                                //If the line is empty then we keep reading
                                if (m_line.has_more()) {
                                    //If the line begins with "<" then it could be a header
                                    //or something else that we do not need to read or
                                    //interpret! Therefore we skip it and read on, otherwise
                                    //it is potentially a meaningful data so we should stop
                                    //and go on to the next section, namely" data
                                    if (m_line[0] != '<') {
                                        LOG_DEBUG1 << "Is something meaningful, moving to data section!" << END_LOG;
                                        break;
                                    } else {
                                        LOG_DEBUG1 << "Is something meaningless, starting with <, skipping forward" << END_LOG;
                                    }
                                } else {
                                    LOG_DEBUG1 << "Is an empty line, skipping forward" << END_LOG;
                                }

                                //Update the progress bar status
                                logger::update_progress_bar();

                                //Read the next line from the file if it is there
                                if (!m_file.get_first_line(m_line)) {
                                    THROW_EXCEPTION("Incorrect ARPA format: An unexpected end of file while reading the ARPA headers!");
                                }
                            }

                            LOG_DEBUG << "Finished reading ARPA headers." << END_LOG;
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::pre_allocate(size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                            LOG_INFO << "Expected number of M-grams per level: "
                                    << array_to_string<size_t, LM_M_GRAM_LEVEL_MAX>(counts) << END_LOG;

                            //Do the progress bard indicator
                            logger::start_progress_bar(string("Pre-allocating memory"));

                            //Provide the N-Gram counts data to the Trie
                            m_trie.pre_allocate(counts);

                            logger::update_progress_bar();

                            LOG_DEBUG << "Finished pre-allocating memory" << END_LOG;
                            //Stop the progress bar in case of no exception
                            logger::stop_progress_bar();
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::read_data(size_t counts[LM_M_GRAM_LEVEL_MAX]) {
                            LOG_DEBUG << "Start reading ARPA data." << END_LOG;

                            //If we are here then it means we just finished reading the
                            //ARPA headers and we stumbled upon something meaningful,
                            //that actually must be the begin of the data section
                            if (m_line != END_OF_ARPA_FILE) {
                                phrase_length level = M_GRAM_LEVEL_1;
                                while (true) {
                                    if (m_file.get_first_line(m_line)) {
                                        LOG_DEBUG1 << "Read data (?) line: '" << m_line.str() << "'" << END_LOG;

                                        //Update the progress bar status
                                        logger::update_progress_bar();

                                        //If the line is empty then we keep reading
                                        if (m_line.has_more()) {
                                            //Check that the next line contains the meaningful N-gram amount information!
                                            if (regex_match(m_line.str(), m_ng_amount_reg_exp)) {
                                                //This is a valid data section entry, there is no need to do anything with it.
                                                //Later we might want to read the numbers and then check them against the
                                                //actual number of provided n-grams but for now it is not needed. 
                                                LOG_DEBUG1 << "Reading " << level << "-gram amount, got: '" << m_line << "'!" << END_LOG;

                                                //Read the number of N-grams in order to have enough data
                                                //for the pre-allocation of the memory in the trie! For
                                                //large language models that should both improve the memory
                                                //usage and the performance of adding new N-Grams!

                                                //First tokenize the string
                                                vector<string> elems;
                                                tokenize(m_line.str(), elems, NGRAM_COUNTS_DELIM);

                                                //Parse the second (last) value and store it as the amount
                                                string & amount = *(--elems.end());
                                                try {
                                                    counts[level - 1] = stoull(amount);
                                                    LOG_DEBUG << "Expected number of " << level << "-grams is: " << counts[level - 1] << END_LOG;
                                                } catch (invalid_argument) {
                                                    stringstream msg;
                                                    msg << "Incorrect ARPA format: Can not parse the "
                                                            << level << "-gram amount: '" << amount
                                                            << "' from string: '" << m_line << "'";
                                                    THROW_EXCEPTION(msg.str());
                                                }
                                            } else {
                                                LOG_DEBUG1 << "Is something other than n-gram amount, moving to n-gram sections!" << END_LOG;
                                                break;
                                            }
                                        } else {
                                            LOG_DEBUG1 << "Is an empty line, skipping forward" << END_LOG;
                                        }
                                    } else {
                                        THROW_EXCEPTION("Incorrect ARPA format: An unexpected end of file while reading the ARPA data section!");
                                    }
                                    level++;
                                }
                            } else {
                                stringstream msg;
                                msg << "Incorrect ARPA format: Got '" << m_line << "' instead of '"
                                        << END_OF_ARPA_FILE << "' when starting on the data section!";
                                THROW_EXCEPTION(msg.str());
                            }

                            LOG_DEBUG << "Finished reading ARPA data." << END_LOG;
                        }

                        template<typename trie_type, typename reader_type>
                        template<phrase_length CURR_LEVEL, bool is_mult_weight>
                        void lm_basic_builder<trie_type, reader_type>::read_m_gram_level() {
                            //Declare the pointer to the N-Grma builder
                            lm_gram_builder<WordIndexType, CURR_LEVEL, is_mult_weight> *gram_builder_ptr = NULL;
                            lm_gram_builder_factory<trie_type>::template get_builder<CURR_LEVEL, is_mult_weight>(m_params, m_trie, &gram_builder_ptr);

                            try {
                                //The counter of the N-grams
                                uint numNgrams = 0;
                                //Read the current level N-grams and add them to the trie
                                while (true) {
                                    //Try to read the next line
                                    if (m_file.get_first_line(m_line)) {
                                        LOG_DEBUG1 << "Read " << CURR_LEVEL << "-Gram (?) line: '" << m_line.str() << "'" << END_LOG;

                                        //Empty lines will just be skipped
                                        if (m_line.has_more()) {
                                            //Pass the given N-gram string to the N-Gram Builder. If the
                                            //N-gram is not matched then stop the loop and move on
                                            if (gram_builder_ptr->parse_line(m_line)) {
                                                //If there was no match then it is something else
                                                //than the given level N-gram so we move on
                                                LOG_DEBUG << "Actual number of " << CURR_LEVEL << "-grams is: " << numNgrams << END_LOG;

                                                //Now stop reading this level N-grams and move on
                                                break;
                                            }
                                            numNgrams++;
                                        }

                                        //Update the progress bar status
                                        logger::update_progress_bar();
                                    } else {
                                        //If the next line does not exist then it an error as we expect the end of data section any way
                                        stringstream msg;
                                        msg << "Incorrect ARPA format: Unexpected end of file, missing the '" << END_OF_ARPA_FILE << "' tag!";
                                        THROW_EXCEPTION(msg.str());
                                    }
                                }
                            } catch (...) {
                                //Free the allocated N-gram builder in case of an exception 
                                delete gram_builder_ptr;
                                //Re-throw an exception, do not use the exception object as it would
                                //create a copy of it loosing all needed additional information.
                                throw;
                            }
                            //Free the allocated N-gram builder in case of no exception 
                            delete gram_builder_ptr;
                            gram_builder_ptr = NULL;

                            LOG_DEBUG << "Finished reading ARPA " << CURR_LEVEL << "-Grams." << END_LOG;
                            //Stop the progress bar in case of no exception
                            logger::stop_progress_bar();
                        }

                        template<typename trie_type, typename reader_type>
                        template<phrase_length CURR_LEVEL>
                        void lm_basic_builder<trie_type, reader_type>::do_post_m_gram_actions() {
                            //Check if the post gram actions are needed! If yes - perform.
                            if (m_trie.template is_post_grams<CURR_LEVEL>()) {
                                //Do the progress bard indicator
                                stringstream msg;
                                msg << "Cultivating " << CURR_LEVEL << "-Grams";
                                logger::start_progress_bar(msg.str());

                                //Do the post level actions
                                m_trie.template post_grams<CURR_LEVEL>();

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();
                                LOG_DEBUG << "Finished post actions of " << CURR_LEVEL << "-Grams." << END_LOG;
                            } else {
                                LOG_INFO3 << "Cultivating " << CURR_LEVEL << "-Grams:\t Not needed!" << END_LOG;
                            }
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::get_word_counts() {
                            //Check if we need another pass for words counting.
                            if (m_trie.get_word_index().is_word_counts_needed()) {
                                //Do the progress bard indicator
                                logger::start_progress_bar(string("Counting all words"));

                                //Start recursive counting of words
                                get_word_counts_from_unigrams();
                                LOG_DEBUG1 << "Finished counting words in M-grams!" << END_LOG;

                                //Perform the post counting actions;
                                m_trie.get_word_index().do_post_word_count();

                                LOG_DEBUG << "Finished counting all words" << END_LOG;
                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();

                                //Rewind to the beginning of the 1-grams section
                                return_to_grams();
                            }
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::do_word_index_post_1_gram_actions() {
                            //Perform the post actions if needed
                            if (m_trie.get_word_index().is_post_actions_needed()) {
                                //Do the progress bard indicator
                                logger::start_progress_bar(string("Word Index actions"));

                                LOG_DEBUG << "Starting to perform the Word Index post actions" << END_LOG;

                                //Perform the post actions
                                m_trie.get_word_index().do_post_actions();

                                LOG_DEBUG << "Finished performing the Word Index post actions" << END_LOG;

                                //Stop the progress bar in case of no exception
                                logger::stop_progress_bar();
                            }
                        }

                        template<typename trie_type, typename reader_type>
                        template<phrase_length CURR_LEVEL>
                        void lm_basic_builder<trie_type, reader_type>::read_grams() {
                            stringstream msg;
                            //Do the progress bard indicator
                            msg << "Reading ARPA " << CURR_LEVEL << "-Grams";
                            logger::start_progress_bar(msg.str());

                            //The regular expression for matching the n-grams section
                            stringstream regexpStr;
                            regexpStr << "\\\\" << CURR_LEVEL << "\\-grams\\:";
                            LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                            const regex n_gram_sect_reg_exp(regexpStr.str());

                            //Check if the line that was input is the header of the N-grams section for N=level
                            if (regex_match(m_line.str(), n_gram_sect_reg_exp)) {
                                //Check if we need to multiply with the m-gram weight
                                if (m_params.m_is_0_lm_weight) {
                                    //Read the M-grams of the given level
                                    read_m_gram_level<CURR_LEVEL, true>();
                                } else {
                                    //Read the M-grams of the given level
                                    read_m_gram_level<CURR_LEVEL, false>();
                                }

                                //If the first M-gram level has been read then do
                                //the word index post-actions if needed.
                                if (CURR_LEVEL == M_GRAM_LEVEL_1) {
                                    do_word_index_post_1_gram_actions();
                                }

                                //Perform the post-M-gram actions if needed
                                do_post_m_gram_actions<CURR_LEVEL>();

                                //Check if we need to keep reading and recurse or we are done
                                check_and_go_m_grams<CURR_LEVEL>();
                            } else {
                                //The obtained string is something else than the next n-grams section header
                                //So the only thing it is allowed to be is the end of file, let's check on
                                //it and otherwise report an error

                                ASSERT_CONDITION_THROW((m_line != END_OF_ARPA_FILE),
                                        string("Incorrect ARPA format: Got '") + m_line.str() +
                                        string("' when trying to read the ") + to_string(CURR_LEVEL) +
                                        string("-grams section!"));
                            }
                        }

                        /**
                         * If the word counts are needed then we are starting
                         * on reading the M-gram section of the ARPA file.
                         * All we need to do is then read all the words in
                         * M-Gram sections and count them with the word index.
                         */
                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::get_word_counts_from_unigrams() {
                            typename trie_type::WordIndexType & word_index = m_trie.get_word_index();

                            //The regular expression for matching the n-grams section
                            stringstream regexpStr;
                            regexpStr << "\\\\" << M_GRAM_LEVEL_1 << "\\-grams\\:";
                            LOG_DEBUG1 << "The N-gram section reg-exp: '" << regexpStr.str() << "'" << END_LOG;
                            const regex n_gram_sect_reg_exp(regexpStr.str());

                            //Check if the line that was input is the header of the N-grams section for N=level
                            if (regex_match(m_line.str(), n_gram_sect_reg_exp)) {
                                //Declare the variables needed to get the word counts
                                text_piece_reader word;
                                prob_weight prob;

                                //Read the current level N-grams and add them to the trie
                                while (m_file.get_first_line(m_line)) {
                                    LOG_DEBUG1 << "Reading " << SSTR(M_GRAM_LEVEL_1) << "-gram, got: [" << m_line.str() << "]" << END_LOG;
                                    //If this is not an empty line
                                    if (m_line.has_more()) {
                                        //Parse line to words without probabilities and back-offs
                                        //If it is not the M-gram line then we stop break
                                        if (lm_gram_builder<WordIndexType, M_GRAM_LEVEL_1, false>::unigram_to_prob(m_line, word, prob)) {
                                            //Set the word with its probability into the word index
                                            word_index.count_word(word, prob);
                                            //Update the progress bar status
                                            logger::update_progress_bar();
                                        } else {
                                            //This is not an expected M-gram
                                            LOG_DEBUG1 << "Stopping words counting in M-gram level: " << SSTR(M_GRAM_LEVEL_1) << END_LOG;
                                            break;
                                        }
                                    }
                                }
                                LOG_DEBUG3 << "Line : " << m_line.str() << END_LOG;
                                LOG_DEBUG1 << "Finished counting words in " << SSTR(M_GRAM_LEVEL_1) << "-grams" << END_LOG;
                            } else {

                                THROW_EXCEPTION("Could not count words, did not get a match with for the beginning of the 1-gram section!");
                            }

                            //Update the progress bar status
                            logger::update_progress_bar();
                        }

                        /**
                         * This method should be called after the word counting is done.
                         * Then we can re-start reading the file and move forward to the
                         * one gram section again. After this method we can proceed
                         * reading M-grams and add them to the trie.
                         */
                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::return_to_grams() {
                            //Reset the file

                            m_file.reset();

                            //Read the first line from the file
                            m_file.get_first_line(m_line);

                            //Skip on ARPA headers
                            read_headers();

                            //Read the DATA section of ARPA
                            size_t counts[LM_M_GRAM_LEVEL_MAX];
                            memset(counts, 0, LM_M_GRAM_LEVEL_MAX * sizeof (size_t));
                            read_data(counts);
                        }

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::set_def_unk_word_prob() {
                            //Set the default unk word probability weight
                            prob_weight unk_prob = m_params.m_unk_word_log_e_prob;

                            LOG_DEBUG << "LM default UNK word prob weight: " << unk_prob << END_LOG;

                            //Check if there is a lambda to multiply with
                            if (m_params.m_is_0_lm_weight) {
                                LOG_DEBUG << "LM weight: " << m_params.get_0_lm_weight() << END_LOG;
                                unk_prob *= m_params.get_0_lm_weight();
                            }
                            
                            //Set the probability weight
                            m_trie.set_def_unk_word_prob(unk_prob);
                        }

                        //Iterate through the ARPA file and fill in the back-off model of the trie
                        //Note that, this file reader will be made ads flexible as possible,
                        //in other words is will ignore as much data as possible and will report
                        //as few errors as possible, mostly warnings. Also the Maximum N-Gram Level
                        //will be limited by the N parameter provided to the class template and not
                        //the maximum N-gram level present in the file.

                        template<typename trie_type, typename reader_type>
                        void lm_basic_builder<trie_type, reader_type>::build() {
                            LOG_DEBUG << "Starting to read the file and build the trie ..." << END_LOG;

                            //Declare an array of N-Gram counts, that is to be filled from the
                            //headers. This data will be used to pre-allocate memory for the Trie 
                            size_t counts[LM_M_GRAM_LEVEL_MAX];
                            memset(counts, 0, LM_M_GRAM_LEVEL_MAX * sizeof (size_t));

                            try {
                                //Read the first line from the file
                                m_file.get_first_line(m_line);

                                //Skip on ARPA headers
                                read_headers();

                                //Read the DATA section of ARPA
                                read_data(counts);

                                //Pre-allocate memory
                                pre_allocate(counts);

                                //Get the word counts, if needed
                                get_word_counts();

                                //Set the default UNK word data
                                set_def_unk_word_prob();

                                //Read the N-grams, starting from 1-Grams
                                read_grams<M_GRAM_LEVEL_1>();
                            } catch (...) {
                                //Stop the progress bar in case of an exception
                                logger::stop_progress_bar();
                                throw;
                            }

                            LOG_DEBUG << "Done reading the file and building the trie." << END_LOG;
                        }

                        //Make sure that there will be templates instantiated, at least for the given parameter values
#define INSTANTIATE_TRIE_BUILDER_FILE_READER(reader_type) \
                template class lm_basic_builder<TC2DMapTrieBasic, reader_type>; \
                template class lm_basic_builder<TC2DMapTrieCount, reader_type>; \
                template class lm_basic_builder<TC2DMapTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TC2DMapTrieOptCount, reader_type>; \
                template class lm_basic_builder<TC2DMapTrieHashing, reader_type>; \
                template class lm_basic_builder<TW2CHybridTrieBasic, reader_type>; \
                template class lm_basic_builder<TW2CHybridTrieCount, reader_type>; \
                template class lm_basic_builder<TW2CHybridTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TW2CHybridTrieOptCount, reader_type>; \
                template class lm_basic_builder<TW2CHybridTrieHashing, reader_type>; \
                template class lm_basic_builder<TC2WArrayTrieBasic, reader_type>; \
                template class lm_basic_builder<TC2WArrayTrieCount, reader_type>; \
                template class lm_basic_builder<TC2WArrayTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TC2WArrayTrieOptCount, reader_type>; \
                template class lm_basic_builder<TC2WArrayTrieHashing, reader_type>; \
                template class lm_basic_builder<TW2CArrayTrieBasic, reader_type>; \
                template class lm_basic_builder<TW2CArrayTrieCount, reader_type>; \
                template class lm_basic_builder<TW2CArrayTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TW2CArrayTrieOptCount, reader_type>; \
                template class lm_basic_builder<TW2CArrayTrieHashing, reader_type>; \
                template class lm_basic_builder<TC2DHybridTrieBasic, reader_type>; \
                template class lm_basic_builder<TC2DHybridTrieCount, reader_type>; \
                template class lm_basic_builder<TC2DHybridTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TC2DHybridTrieOptCount, reader_type>; \
                template class lm_basic_builder<TC2DHybridTrieHashing, reader_type>; \
                template class lm_basic_builder<TG2DMapTrieBasic, reader_type>; \
                template class lm_basic_builder<TG2DMapTrieCount, reader_type>; \
                template class lm_basic_builder<TG2DMapTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TG2DMapTrieOptCount, reader_type>; \
                template class lm_basic_builder<TG2DMapTrieHashing, reader_type>; \
                template class lm_basic_builder<TH2DMapTrieBasic, reader_type>; \
                template class lm_basic_builder<TH2DMapTrieCount, reader_type>; \
                template class lm_basic_builder<TH2DMapTrieOptBasic, reader_type>; \
                template class lm_basic_builder<TH2DMapTrieOptCount, reader_type>; \
                template class lm_basic_builder<TH2DMapTrieHashing, reader_type>;

                        INSTANTIATE_TRIE_BUILDER_FILE_READER(cstyle_file_reader);
                        INSTANTIATE_TRIE_BUILDER_FILE_READER(file_stream_reader);
                        INSTANTIATE_TRIE_BUILDER_FILE_READER(memory_mapped_file_reader);
                    }
                }
            }
        }
    }
}