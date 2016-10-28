/* 
 * File:   BasicWordIndex.hpp
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
 * Created on August 21, 2015, 12:41 PM
 */

#ifndef BASICWORDINDEX_HPP
#define BASICWORDINDEX_HPP

#include <string>         // std::string
#include <unordered_map>  // std::unordered_map

#include "common/utils/logging/logger.hpp"
#include "common/utils/exceptions.hpp"
#include "common/utils/hashing_utils.hpp"
#include "common/utils/file/text_piece_reader.hpp"
#include "common/utils/containers/greedy_memory_allocator.hpp"

#include "server/lm/lm_consts.hpp"
#include "server/lm/dictionaries/aword_index.hpp"

using namespace std;
using namespace uva::utils::file;
using namespace uva::utils::exceptions;
using namespace uva::utils::containers::alloc;
using namespace uva::smt::bpbd::server::lm::identifiers;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace server {
                namespace lm {
                    namespace dictionary {

                        /**
                         * This is a hash-map based implementation of the word index.
                         */
                        class basic_word_index : public aword_index {
                        public:

                            /**
                             * The basic constructor
                             * @param  wordIndexMemFactor the assigned memory factor for
                             * storage allocation in the unordered_map used for the word index
                             */
                            basic_word_index(const float wordIndexMemFactor)
                            : m_word_index_alloc_ptr(NULL), m_word_index_map_ptr(NULL), m_next_new_word_id(MIN_KNOWN_WORD_ID), m_word_index_mem_factor(wordIndexMemFactor) {
                            };

                            /**
                             * Allows to get the total words count including the unknown and undefined words
                             * @see AWordIndex
                             */
                            inline size_t get_number_of_words(const size_t num_words) const {
                                return num_words + EXTRA_NUMBER_OF_WORD_IDs;
                            };

                            /**
                             * This method should be used to pre-allocate the word index
                             * @see AWordIndex
                             */
                            inline void reserve(const size_t num_words) {
                                //Compute the number of words to be stored
                                //Add an extra elements for the <unknown/> word
                                const size_t numWords = get_number_of_words(num_words);

                                //Reserve the memory for the map
                                reserve_mem_unordered_map<TWordIndexMap, TWordIndexAllocator>(&m_word_index_map_ptr, &m_word_index_alloc_ptr,
                                        numWords, "WordIndex", m_word_index_mem_factor);

                                //Register the unknown word with the first available hash value
                                word_uid& word_id = m_word_index_map_ptr->operator[](LM_UNKNOWN_WORD_STR);
                                word_id = UNKNOWN_WORD_ID;
                            };

                            /**
                             * This function gets an id for the given word word based no the stored 1-Grams.
                             * If the word is not known then an unknown word ID is returned: UNKNOWN_WORD_ID
                             * @see AWordIndex
                             */
                            inline word_uid get_word_id(const text_piece_reader & token) const {
                                TWordIndexMapConstIter result = m_word_index_map_ptr->find(token.str());
                                if (result == m_word_index_map_ptr->end()) {
                                    LOG_DEBUG << "Word: '" << token << "' is not known! Mapping it to: '"
                                            << LM_UNKNOWN_WORD_STR << "', id: "
                                            << SSTR(UNKNOWN_WORD_ID) << END_LOG;
                                    return UNKNOWN_WORD_ID;
                                } else {
                                    return result->second;
                                }
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
                            inline word_uid register_word(const text_piece_reader & token) {
                                //First get/create an existing/new word entry from from/in the word index
                                word_uid& hash = m_word_index_map_ptr->operator[](token.str());

                                if (hash == UNDEFINED_WORD_ID) {
                                    //If the word hash is not defined yet, then issue it a new hash id
                                    hash = m_next_new_word_id++;
                                    LOG_DEBUG2 << "Word: '" << token.str() << "' is not known yet, issuing it a new id: " << SSTR(hash) << END_LOG;
                                }

                                //Use the Prime numbers hashing algorithm as it outperforms djb2
                                return hash;
                            }

                            /**
                             * This method allows to indicate whether word counting is
                             * needed by the given implementation of the word index.
                             * @see AWordIndex
                             */
                            inline bool is_word_counts_needed() const {
                                return false;
                            };

                            /**
                             * This method is to be used when the word counting is needed.
                             * @see AWordIndex
                             */
                            inline void count_word(const text_piece_reader & word, prob_weight prob) {
                                //There is nothing to be done
                                THROW_MUST_NOT_CALL();
                            };

                            /**
                             * Should be called if the word count is needed
                             * after all the words have been counted.
                             * @see AWordIndex
                             */
                            inline void do_post_word_count() {
                                //There is nothing to be done
                                THROW_MUST_NOT_CALL();
                            };

                            /**
                             * Indicates if the post-actions are needed. The post actions
                             * should be called after all the words have been filled into
                             * the index.
                             * @see AWordIndex
                             */
                            inline bool is_post_actions_needed() const {
                                return false;
                            };

                            /**
                             * Is to be called if the post actions are needed right after
                             * that all the individual words have been added into the index.
                             * @see AWordIndex
                             */
                            inline void do_post_actions() {
                                //There is nothing to be done
                                THROW_MUST_NOT_CALL();
                            };

                            /**
                             * Allows to indicate if the word index is continuous, i.e.
                             * it issues the word ids in a continuous range starting from 0.
                             * @see AWordIndex
                             * @return true - this word index is continuous.
                             */
                            static constexpr inline bool is_word_index_continuous() {
                                return true;
                            }

                            /**
                             * The basic destructor
                             */
                            virtual ~basic_word_index() {
                                deallocate_container<TWordIndexMap, TWordIndexAllocator>(&m_word_index_map_ptr, &m_word_index_alloc_ptr);
                            };

                            /**
                             * The type of key,value pairs to be stored in the word index
                             */
                            typedef pair< const string, word_uid> TWordIndexEntry;

                            /**
                             * The typedef for the word index allocator
                             */
                            typedef greedy_memory_allocator< TWordIndexEntry > TWordIndexAllocator;

                            /**
                             * The word index map type
                             */
                            typedef unordered_map<string, word_uid, std::hash<string>, std::equal_to<string>, TWordIndexAllocator > TWordIndexMap;

                            /**
                             * Defines the constant iterator type
                             */
                            typedef TWordIndexMap::const_iterator TWordIndexMapConstIter;

                            /**
                             * Allows to get the begin constant iterator
                             * @return the begin constant iterator
                             */
                            TWordIndexMapConstIter begin() {
                                return m_word_index_map_ptr->begin();
                            }

                            /**
                             * Allows to get the end constant iterator
                             * @return the end constant iterator
                             */
                            TWordIndexMapConstIter end() {
                                return m_word_index_map_ptr->end();
                            }

                        protected:

                            /**
                             * The copy constructor, is made private as we do not intend to copy this class objects
                             * @param orig the object to copy from
                             */
                            basic_word_index(const basic_word_index & other)
                            : m_word_index_alloc_ptr(NULL), m_word_index_map_ptr(NULL),
                            m_next_new_word_id(MIN_KNOWN_WORD_ID), m_word_index_mem_factor(0.0) {
                                THROW_EXCEPTION("HashMapWordIndex copy constructor is not to be used, unless implemented!");
                            }
                            //This is the pointer to the fixed memory allocator used to allocate the map's memory
                            TWordIndexAllocator * m_word_index_alloc_ptr;

                            //This map stores the word index, i.e. assigns each unique word a unique id
                            TWordIndexMap * m_word_index_map_ptr;

                            //Stores the last allocated word hash
                            word_uid m_next_new_word_id;

                            //Stores the assigned memory factor for storage allocation
                            //in the unordered_map used for the word index
                            const float m_word_index_mem_factor;

                        };
                    }
                }
            }
        }
    }
}


#endif /* HASHMAPWORDINDEX_HPP */

