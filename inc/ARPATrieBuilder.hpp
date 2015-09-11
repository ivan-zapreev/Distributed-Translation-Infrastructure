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

#ifndef TRIEBUILDER_HPP
#define	TRIEBUILDER_HPP

#include <fstream>      // std::ifstream
#include <regex>        // std::regex, std::regex_match

#include "ATrie.hpp"

#include "TextPieceReader.hpp"
#include "AFileReader.hpp"

using namespace std;
using namespace uva::smt::file;

namespace uva {
    namespace smt {
        namespace tries {
            namespace arpa {

                /**
                 * This is the Trie builder class that reads an input file stream
                 * and creates n-grams and then records them into the provided Trie.
                 */
                template<TModelLevel N>
                class ARPATrieBuilder {
                public:
                    /**
                     * The basic constructor that accepts a trie to be build up and the file stream to read from
                     * @param trie the trie to fill in with data from the text corpus
                     * @param _fstr the file stream to read from
                     */
                    ARPATrieBuilder(ATrie<N> & trie, AFileReader & file);

                    /**
                     * This function will read from the file and build the trie
                     */
                    void build();

                    virtual ~ARPATrieBuilder();
                private:
                    //The reference to the trie to be build
                    ATrie<N> & m_trie;
                    //The reference to the input file with language model
                    AFileReader & m_file;
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
                    ARPATrieBuilder(const ARPATrieBuilder& orig);

                    /**
                     * This method is used to read and process the ARPA headers
                     * @param line the in/out parameter storing the last read line
                     */
                    void read_headers();

                    /**
                     * Allows to pre-allocate memory in the tries and dictionaries
                     * @param counts the learned M-gram counts
                     */
                    void pre_allocate(size_t counts[N]);

                    /**
                     * This method is used to read and process the ARPA data section
                     * @param line the in/out parameter storing the last read line
                     * @param counts the out parameters to store the retrieved
                     *               N-Gram counts from the data descriptions
                     */
                    void read_data(size_t counts[N]);

                    /**
                     * If the word counts are needed then we are starting
                     * on reading the M-gram section of the ARPA file.
                     * All we need to do is then read all the words in
                     * M-Gram sections and count them with the word index.
                     */
                    void get_word_counts() {
                        //Do the progress bard indicator
                        Logger::startProgressBar(string("Counting all words"));

                        //Start recursive counting of words
                        get_word_counts(1);
                        LOG_INFO << "Finished counting words in M-grams!" << END_LOG;

                        //Perform the post counting actions;
                        m_trie.get_word_index()->post_word_count();

                        LOG_DEBUG << "Finished counting all words" << END_LOG;
                        //Stop the progress bar in case of no exception
                        Logger::stopProgressBar();

                    }

                    /**
                     * If the word counts are needed then we are starting
                     * on reading the M-gram section of the ARPA file.
                     * All we need to do is then read all the words in
                     * M-Gram sections and count them with the word index.
                     * @param level the M-gram level we are counting words for.
                     */
                    void get_word_counts(const TModelLevel level);

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
                    void read_grams(const TModelLevel level);
                };
            }
        }
    }
}
#endif	/* TRIEBUILDER_HPP */

