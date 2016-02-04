/* 
 * File:   CountingWordIndex.hpp
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
 * Created on September 6, 2015, 2:49 PM
 */

#ifndef COUNTINGWORDINDEX_HPP
#define COUNTINGWORDINDEX_HPP

#include <string>   // std::string

#include "BasicWordIndex.hpp"

#include "server/lm/TrieConstants.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"

#include "common/utils/containers/array_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::utils::exceptions;
using namespace uva::utils::containers::utils;

namespace uva {
    namespace smt {
        namespace translation {
            namespace server {
                namespace lm {
                    namespace dictionary {

                        namespace __CountingWordIndex {
                            //This structure is used to store the word 
                            //information such as the word and its probability

                            typedef struct {
                                string word;
                                TLogProbBackOff prob;
                            } TWordInfo;

                            /**
                             * The comparison operator for two word info objects,
                             * the one that is smaller has the highest word probability.
                             * @param one the first object to compare
                             * @param two the second object to compare
                             * @return the smaller one is the most used one, with the higher word probability
                             */
                            inline bool operator<(const TWordInfo & one, const TWordInfo & two) {
                                return (one.prob > two.prob);
                            }
                        }

                        /**
                         * This is a hash-map based implementation of the word index
                         * which extends the basic word index by word counting. This
                         * allows to count the word usages and then to issue lower
                         * word indexes to the more frequently used words. This allows
                         * for, for example, shorter M-gram ids.
                         * 
                         * ToDo: Change or create a new version of the word index that will
                         * just use probabilities of the unigrams instead of counting words.
                         */
                        class CountingWordIndex : public BasicWordIndex {
                        public:

                            /**
                             * The basic constructor
                             * @param  wordIndexMemFactor the assigned memory factor for
                             * storage allocation in the unordered_map used for the word index
                             */
                            CountingWordIndex(const float wordIndexMemFactor) : BasicWordIndex(wordIndexMemFactor) {
                                ASSERT_CONDITION_THROW(BasicWordIndex::is_word_counts_needed(),
                                        "The BasicWordIndex must needs word counts! Update CountingWordIndex!");
                                ASSERT_CONDITION_THROW(sizeof (TWordIdType) != sizeof (TLogProbBackOff),
                                        "The same size TWordIdType and TLogProbBackOff types are required!");
                            }

                            /**
                             * This method allows to indicate whether registering a word is
                             * needed by the given implementation of the word index.
                             * @see AWordIndex
                             */
                            inline bool is_word_registering_needed() const {
                                return true;
                            };

                            /**
                             * This function creates/gets a hash for the given word.
                             * @see AWordIndex
                             */
                            TWordIdType register_word(const TextPieceReader & token) {
                                //Note that, by now all the words must have been counted
                                //and have their unique words ids, so here we do it simple!
                                //Return the id that has already been issued!
                                return m_word_index_map_ptr->at(token.str());
                            }

                            /**
                             * This method is to be used when the word counting is needed.
                             * @see AWordIndex
                             */
                            inline void count_word(const TextPieceReader & word, TLogProbBackOff prob) {
                                //Misuse the internal word index map for storing the word counts in it.
                                LOG_DEBUG1 << "Adding the word: '" << word.str() << "', with prob: " << prob << END_LOG;
                                TWordIdType value;
                                memcpy(&value, &prob, sizeof (TWordIdType));
                                BasicWordIndex::m_word_index_map_ptr->operator[](word.str()) = value;
                            };

                            /**
                             * This method allows to indicate whether word counting is
                             * needed by the given implementation of the word index.
                             * @see AWordIndex
                             */
                            bool is_word_counts_needed() const {
                                return true;
                            };

                            /**
                             * Should be called if the word count is needed
                             * after all the words have been counted.
                             * @see AWordIndex
                             */
                            void do_post_word_count() {
                                //All the words have been filled in, it is time to give them ids.
                                LOG_DEBUG1 << "Starting the post word counting actions!" << END_LOG;

                                //00. Remove the <unk> word from the Map as it must get fixed index
                                LOG_DEBUG2 << "Remove the <unk> word from the Map as it must get fixed index" << END_LOG;
                                BasicWordIndex::m_word_index_map_ptr->erase(AWordIndex::UNKNOWN_WORD_STR);

                                //01. Create an array of words info objects from BasicWordIndex::_pWordIndexMap
                                LOG_DEBUG2 << "Create an array of words info objects from BasicWordIndex::_pWordIndexMap" << END_LOG;
                                const size_t num_words = BasicWordIndex::m_word_index_map_ptr->size();
                                __CountingWordIndex::TWordInfo * word_infos = new __CountingWordIndex::TWordInfo[num_words];

                                //02. Copy the word information from the map into that array.
                                LOG_DEBUG2 << "Copy the word information from the map into that array." << END_LOG;
                                BasicWordIndex::TWordIndexMap::const_iterator iter = BasicWordIndex::m_word_index_map_ptr->begin();
                                for (size_t idx = 0; iter != BasicWordIndex::m_word_index_map_ptr->end(); ++iter, ++idx) {
                                    word_infos[idx].word = iter->first;
                                    memcpy(&word_infos[idx].prob, &iter->second, sizeof (TLogProbBackOff));
                                }

                                //03. Sort the array of word info object in order to get
                                //    the most used words in the beginning of the array
                                LOG_DEBUG2 << "Sort the array of word info object in order to get "
                                        << "the most used words in the beginning of the array" << END_LOG;
                                my_sort<__CountingWordIndex::TWordInfo>(word_infos, num_words);

                                //04. Iterate through the array and assign the new word ids
                                //    into the _pWordIndexMap using the BasicWordIndex::_nextNewWordId
                                LOG_DEBUG2 << "Iterate through the array and assign the new word ids "
                                        << "into the _pWordIndexMap using the BasicWordIndex::_nextNewWordId" << END_LOG;
                                for (size_t idx = 0; idx < num_words; ++idx) {
                                    //Get the next word
                                    string & word = word_infos[idx].word;
                                    //Give it the next index
                                    BasicWordIndex::m_word_index_map_ptr->operator[](word) = BasicWordIndex::m_next_new_word_id++;
                                    LOG_DEBUG4 << "Word [" << word << "], count: " << word_infos[idx].prob << " gets id: "
                                            << SSTR(BasicWordIndex::m_next_new_word_id - 1) << END_LOG;
                                }

                                //05. Delete the temporary sorted array
                                LOG_DEBUG2 << "Delete the temporary sorted array" << END_LOG;
                                delete[] word_infos;

                                //06. Put back the <unk> word with its fixed index into the map
                                LOG_DEBUG2 << "Put back the <unk> word with its fixed index into the map" << END_LOG;
                                BasicWordIndex::m_word_index_map_ptr->operator[](UNKNOWN_WORD_STR) = UNKNOWN_WORD_ID;

                                LOG_DEBUG1 << "Finished the post word counting actions!" << END_LOG;
                            };

                            /**
                             * Indicates if the post-actions are needed. The post actions
                             * should be called after all the words have been filled into
                             * the index.
                             * @see AWordIndex
                             */
                            bool is_post_actions_needed() const {
                                return BasicWordIndex::is_post_actions_needed() || false;
                            };

                            /**
                             * Allows to indicate if the word index is continuous, i.e.
                             * it issues the word ids in a continuous range starting from 0.
                             * @see AWordIndex
                             * @return true - this word index is continuous.
                             */
                            static constexpr inline bool is_word_index_continuous() {
                                return BasicWordIndex::is_word_index_continuous();
                            }

                            /**
                             * Is to be called if the post actions are needed right after
                             * that all the individual words have been added into the index.
                             * @see AWordIndex
                             */
                            void do_post_actions() {
                                //Perform the post actions if needed, before starting further actions.
                                if (BasicWordIndex::is_post_actions_needed()) {
                                    BasicWordIndex::do_post_actions();
                                }

                                //There is nothing to be done
                            };

                        };
                    }
                }
            }
        }
    }
}

#endif /* COUNTINGWORDINDEX_HPP */

