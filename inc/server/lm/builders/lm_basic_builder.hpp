/* 
 * File:   TrieBuilder.hpp
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

#ifndef LM_BASIC_BUILDER_HPP
#define LM_BASIC_BUILDER_HPP

#include <regex>        // std::regex, std::regex_match

#include "server/lm/lm_consts.hpp"
#include "server/lm/lm_parameters.hpp"
#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::smt::bpbd::server::lm;
using namespace uva::smt::bpbd::server::lm::identifiers;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace arpa {

                        //The end of ARPA file constant
                        static const string END_OF_ARPA_FILE = "\\end\\";
                        //The N-gram Data Section Amoung delimiter
                        static const string NGRAM_COUNTS_DELIM = "=";

                        /**
                         * This is the Trie builder class that reads an input file stream
                         * and creates n-grams and then records them into the provided Trie.
                         * This is an ARPA format based trie builder,  so it expects that
                         * the provided model file contains a basic text model in ARPA format.
                         */
                        template<typename TrieType, typename TFileReaderModel>
                        class lm_basic_builder {
                        public:
                            typedef typename TrieType::WordIndexType WordIndexType;

                            /**
                             * The basic constructor that accepts a trie to be build up and the file stream to read from
                             * @params params the model parameters
                             * @param trie the trie to fill in with data from the text corpus
                             * @param _fstr the file stream to read from
                             */
                            lm_basic_builder(const lm_parameters & params, TrieType & trie, TFileReaderModel & file);

                            /**
                             * This function will read from the file and build the trie
                             */
                            void build();

                            virtual ~lm_basic_builder();
                        private:
                            //Stores the reference to the model parameters
                            const lm_parameters & m_params;
                            //The reference to the trie to be build
                            TrieType & m_trie;
                            //The reference to the input file with language model
                            TFileReaderModel & m_file;
                            //Stores the next line data
                            TextPieceReader m_line;
                            //The regular expression for matching the n-gram amount entry of the data section
                            const regex m_ng_amount_reg_exp;
                            //The regular expression for matching the n-grams section
                            const regex m_ng_section_reg_exp;

                            /**
                             * The copy constructor
                             * @param orig the other builder to copy
                             */
                            lm_basic_builder(const lm_basic_builder<TrieType, TFileReaderModel>& orig);

                            /**
                             * This method is used to read and process the ARPA headers
                             * @param line the in/out parameter storing the last read line
                             */
                            void read_headers();

                            /**
                             * Allows to pre-allocate memory in the tries and dictionaries
                             * @param counts the learned M-gram counts
                             */
                            void pre_allocate(size_t counts[LM_M_GRAM_LEVEL_MAX]);

                            /**
                             * This method is used to read and process the ARPA data section
                             * @param line the in/out parameter storing the last read line
                             * @param counts the out parameters to store the retrieved
                             *               N-Gram counts from the data descriptions
                             */
                            void read_data(size_t counts[LM_M_GRAM_LEVEL_MAX]);

                            /**
                             * Allows to read the given Trie level M-grams from the file
                             * @param level the currently read M-gram level M
                             * @param is_mult_weight true if we need to multiply the lm
                             * probabilities and back-off weights with the language model
                             * weight from the model properties
                             */
                            template<phrase_length CURR_LEVEL, bool is_mult_weight>
                            void read_m_gram_level();

                            template<phrase_length CURR_LEVEL, typename DUMMY = void>
                            struct Func {

                                /**
                                 * This template is to be used for the level parameter values < M_GRAM_LEVEL_MAX
                                 */
                                static inline void check_and_go_m_grams(lm_basic_builder<TrieType, TFileReaderModel> & builder) {
                                    //If we expect more N-grams then make a recursive call to read the higher order N-gram
                                    LOG_DEBUG2 << "The currently read N-grams level is " << CURR_LEVEL << ", the maximum level is " << LM_M_GRAM_LEVEL_MAX
                                            << ", the current line is '" << builder.m_line << "'" << END_LOG;

                                    //There are still N-Gram levels to read
                                    if (builder.m_line != END_OF_ARPA_FILE) {
                                        //We did not encounter the \end\ tag yet so do recursion to the next level
                                        builder.read_grams < CURR_LEVEL + 1 > ();
                                    } else {
                                        //We did encounter the \end\ tag, this is not really expected, but it is not fatal
                                        LOG_WARNING << "End of ARPA file, read " << CURR_LEVEL << "-grams and there is "
                                                << "nothing more to read. The maximum allowed N-gram level is " << LM_M_GRAM_LEVEL_MAX << END_LOG;
                                    }
                                }
                            };

                            template<typename DUMMY>
                            struct Func<LM_M_GRAM_LEVEL_MAX, DUMMY> {

                                /**
                                 * This template specialization is to be used for the level parameter values == M_GRAM_LEVEL_MAX
                                 */
                                static inline void check_and_go_m_grams(lm_basic_builder<TrieType, TFileReaderModel> & builder) {
                                    //If we expect more N-grams then make a recursive call to read the higher order N-gram
                                    LOG_DEBUG2 << "The currently read N-grams level is " << LM_M_GRAM_LEVEL_MAX << ", the maximum level is " << LM_M_GRAM_LEVEL_MAX
                                            << ", the current line is '" << builder.m_line << "'" << END_LOG;
                                    //Here the level is >= N, so we must have read a valid \end\ tag, otherwise an error!
                                    if (builder.m_line != END_OF_ARPA_FILE) {
                                        stringstream msg;
                                        msg << "Incorrect ARPA format: Got '" << builder.m_line
                                                << "' instead of '" << END_OF_ARPA_FILE
                                                << "' when reading " << LM_M_GRAM_LEVEL_MAX << "-grams section!";
                                        throw Exception(msg.str());
                                    }
                                }
                            };

                            /**
                             * Checks if this is the last M-gram level we had to read,
                             * if no read more otherwise we are done.
                             * @param level the currently read M-gram level M
                             */
                            template<phrase_length CURR_LEVEL>
                            inline void check_and_go_m_grams() {
                                Func<CURR_LEVEL>::check_and_go_m_grams(*this);
                            }

                            /**
                             * Allows to perform the post-gram actions if needed
                             * @param level the currently read M-gram level M
                             */
                            template<phrase_length CURR_LEVEL>
                            void do_post_m_gram_actions();

                            /**
                             * Allows to perform the word index post-actions if needed
                             */
                            void do_word_index_post_1_gram_actions();

                            /**
                             * If the word counts are needed then this method will 
                             * read all the M-gram sections of the ARPA file to get them.
                             * Afterwards the file will be rewind to the beginning
                             * of the 1-Gram sections.
                             */
                            void get_word_counts();

                            /**
                             * If the word counts are needed then we are starting
                             * on reading the M-gram section of the ARPA file.
                             * All we need to do is then read all the words in
                             * M-Gram sections and count them with the word index.
                             * @param level the M-gram level we are counting words for.
                             */
                            void get_word_counts_from_unigrams();

                            /**
                             * This method should be called after the word counting is done.
                             * Then we can re-start reading the file and move forward to the
                             * one gram section again. After this method we can proceed
                             * reading M-grams and add them to the trie.
                             */
                            void return_to_grams();

                            /**
                             * This recursive method is used to read and process the ARPA N-Grams.
                             * @param line the in/out parameter storing the last read line
                             * @param level the level we are to read
                             */
                            template<phrase_length CURR_LEVEL>
                            void read_grams();
                        };
                    }
                }
            }
        }
    }
}
#endif /* TRIEBUILDER_HPP */

