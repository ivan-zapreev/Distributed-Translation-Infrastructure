/* 
 * File:   ATrie.cpp
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
 * Created on August 25, 2015, 29:27 PM
 */
#include "ALayeredTrie.hpp"

#include "Globals.hpp"
#include "Logger.hpp"
#include "Exceptions.hpp"

namespace uva {
    namespace smt {
        namespace tries {

            template<TModelLevel N>
            void ALayeredTrie<N>::add_1_gram(const T_M_Gram &gram) {
                //First get the token/word from the 1-Gram
                const TextPieceReader & token = gram.tokens[0];

                LOG_DEBUG << "Adding a 1-Gram: '" << token << "' to the Trie." << END_LOG;

                //Compute it's hash value
                TShortId wordHash = ATrie<N>::get_word_index()->register_word(token);
                //Get the word probability and back-off data reference
                TProbBackOffEntry & pbData = make_1_GramDataRef(wordHash);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (pbData.prob != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!

                    REPORT_COLLISION_WARNING(N, gram, wordHash, AWordIndex::UNDEFINED_WORD_ID,
                            pbData.prob, pbData.back_off,
                            gram.prob, gram.back_off);
                }

                //Set/Update the probability and back-off values for the word
                pbData.prob = gram.prob;
                pbData.back_off = gram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.prob << "," << pbData.back_off << ") for "
                        << tokensToString<N>(gram) << " wordHash = "
                        << wordHash << END_LOG;
            };

            template<TModelLevel N>
            void ALayeredTrie<N>::add_m_gram(const T_M_Gram &gram) {
                if (ATrie<N>::m_is_birmap_hash_cache) {
                    //Call the super class first, is needed for caching
                    ATrie<N>::register_m_gram_cache(gram);
                }

                const TModelLevel level = gram.level;
                LOG_DEBUG << "Adding a " << SSTR(level) << "-Gram "
                        << tokensToString<N>(gram) << " to the Trie" << END_LOG;

                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TLongId ctxId;
                bool isFound = getContextId<DebugLevelsEnum::DEBUG2>(gram, ctxId);

                if (DO_SANITY_CHECKS && !isFound) {
                    stringstream msg;
                    msg << "Could not get ctxId for " << tokensToString<N>(gram);
                    throw Exception(msg.str());
                }

                // 2. Compute the hash of w4
                const TextPieceReader & endWord = gram.tokens[level - 1];
                TShortId wordId;
                isFound = ATrie<N>::get_word_index()->get_word_id(endWord.str(), wordId);

                if (DO_SANITY_CHECKS && !isFound) {
                    stringstream msg;
                    msg << "Could not get end wordId for " << tokensToString<N>(gram);
                    throw Exception(msg.str());
                }

                LOG_DEBUG2 << "wordId = computeId('" << endWord.str() << "') = " << wordId << END_LOG;

                // 3. Insert the probability data into the trie
                TProbBackOffEntry& pbData = make_M_GramDataRef(level, wordId, ctxId);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (pbData.prob != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!
                    REPORT_COLLISION_WARNING(N, gram, wordId, ctxId,
                            pbData.prob, pbData.back_off,
                            gram.prob, gram.back_off);
                }

                //Set/Update the probability and back-off values for the word
                pbData.prob = gram.prob;
                pbData.back_off = gram.back_off;

                LOG_DEBUG1 << "Inserted the (prob,back-off) data ("
                        << pbData.prob << "," << pbData.back_off << ") for "
                        << tokensToString<N>(gram) << " contextHash = "
                        << ctxId << ", wordHash = " << wordId << END_LOG;
            };

            template<TModelLevel N>
            void ALayeredTrie<N>::add_n_gram(const T_M_Gram &gram) {
                if (ATrie<N>::m_is_birmap_hash_cache) {
                    //Call the super class first, is needed for caching
                    ATrie<N>::register_m_gram_cache(gram);
                }

                LOG_DEBUG << "Adding a " << N << "-Gram " << tokensToString<N>(gram) << " to the Trie" << END_LOG;

                //To add the new N-gram (e.g.: w1 w2 w3 w4) data inserted, we need to:

                // 1. Compute the context hash defined by w1 w2 w3
                TLongId ctxId;
                bool isFound = getContextId<DebugLevelsEnum::DEBUG2>(gram, ctxId);

                if (DO_SANITY_CHECKS && !isFound) {
                    stringstream msg;
                    msg << "Could not get ctxId for " << tokensToString<N>(gram);
                    throw Exception(msg.str());
                }

                // 2. Compute the hash of w4
                const TextPieceReader & endWord = gram.tokens[N - 1];
                TShortId wordId;
                isFound = ATrie<N>::get_word_index()->get_word_id(endWord.str(), wordId);

                if (DO_SANITY_CHECKS && !isFound) {
                    stringstream msg;
                    msg << "Could not get end wordId for " << tokensToString<N>(gram);
                    throw Exception(msg.str());
                }

                LOG_DEBUG2 << "wordId = computeId('" << endWord << "') = " << wordId << END_LOG;

                // 3. Insert the probability data into the trie
                TLogProbBackOff& pData = make_N_GramDataRef(wordId, ctxId);

                //Check that the probability data is not set yet, otherwise a warning!
                if (DO_SANITY_CHECKS && (pData != ZERO_PROB_WEIGHT)) {
                    //If the probability is not zero then this word has been already seen!

                    REPORT_COLLISION_WARNING(N, gram, wordId, ctxId,
                            pData, UNDEF_LOG_PROB_WEIGHT,
                            gram.prob, UNDEF_LOG_PROB_WEIGHT);
                }

                //Set/Update the probability
                pData = gram.prob;

                LOG_DEBUG1 << "Inserted the prob. data (" << pData << ") for "
                        << tokensToString<N>(gram) << " contextHash = "
                        << ctxId << ", wordHash = " << wordId << END_LOG;
            };

            template<TModelLevel N>
            bool ALayeredTrie<N>::get_back_off_weight(const TModelLevel level, TLogProbBackOff & back_off) {
                //Get the word hash for the en word of the back-off N-Gram
                const TShortId & wordId = ATrie<N>::get_back_off_end_word_id();
                const TModelLevel boCtxLen = level - 1;

                LOG_DEBUG << "Computing back-off for an " << level
                        << "-gram the context length is " << boCtxLen << END_LOG;

                if (boCtxLen > 0) {
                    //Attempt to retrieve back-off weights
                    TLongId ctxId = AWordIndex::UNDEFINED_WORD_ID;
                    //Compute the context hash
                    if (get_query_context_Id<true>(boCtxLen, ctxId)) {
                        LOG_DEBUG << "Got query context id: " << ctxId << END_LOG;

                        //The context length plus one is M value of the M-Gram
                        const TProbBackOffEntry * pEntry;
                        if (get_M_GramDataRef(level, wordId, ctxId, &pEntry)) {
                            //Obtained the stored back-off weight
                            back_off = pEntry->back_off;

                            LOG_DEBUG << "The " << level << "-Gram log_"
                                    << LOG_PROB_WEIGHT_BASE << "( back-off ) for (wordId, ctxId)=("
                                    << wordId << ", " << ctxId << "), is: " << back_off << END_LOG;

                            return true;
                        } else {
                            //The query context id could be determined, but 
                            //the data was not found in the trie.
                            LOG_DEBUG << "Unable to find data for " << (level)
                                    << "-Gram query with end wordId: "
                                    << SSTR(wordId) << ", ctxId: "
                                    << SSTR(ctxId) << "!" << END_LOG;
                            return false;
                        }
                    } else {
                        //The query context id could not be determined,
                        //so the M-gram is not present!
                        LOG_DEBUG << "Unable to find ctxId for " << (level)
                                << "-Gram query with end wordId: "
                                << SSTR(wordId) << "!" << END_LOG;
                        return false;
                    }
                } else {
                    //We came to a zero context, which means we have an
                    //1-Gram to try to get the back-off weight from
                    //Attempt to retrieve back-off weights
                    const TProbBackOffEntry * pbData;
                    if (get_1_GramDataRef(wordId, &pbData)) {
                        //Note that: If the stored back-off is UNDEFINED_LOG_PROB_WEIGHT then the back of is just zero
                        back_off = pbData->back_off;

                        LOG_DEBUG << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( back-off ) for word: " << wordId
                                << ", is: " << back_off << END_LOG;
                        return true;
                    } else {
                        //The one gram data is not present!
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << wordId << ", nowhere to back-off!" << END_LOG;
                        return false;
                    }
                }

                LOG_DEBUG << "The chosen log back-off weight for context: " << level << " is: " << back_off << END_LOG;

                //Return the computed back-off weight it can be UNDEFINED_LOG_PROB_WEIGHT, which is zero - no penalty

                return back_off;
            }

            template<TModelLevel N>
            void ALayeredTrie<N>::get_prob_value(const TModelLevel level, TLogProbBackOff & prob) {
                //Compute the context length of the given M-Gram
                const TModelLevel ctxLen = level - 1;
                //Get the last word in the N-gram
                const TShortId & wordId = ATrie<N>::get_end_word_id();

                LOG_DEBUG << "Computing probability for an " << level
                        << "-gram the context length is " << ctxLen << END_LOG;

                //Consider different variants based no the length of the context
                if (ctxLen > 0) {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram
                    TLongId ctxId;

                    //Compute the context id based on what is stored in m_GramWordIds and context length
                    if (get_query_context_Id<false>(ctxLen, ctxId)) {

                        LOG_DEBUG << "Got query context id: " << ctxId << END_LOG;

                        if (level == N) {
                            //If we are looking for a N-Gram probability
                            if (get_N_GramProb(wordId, ctxId, prob)) {
                                LOG_DEBUG2 << "The " << N << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                        << "( prob. ) for (wordId,ctxId) = (" << wordId << ", "
                                        << ctxId << "), is: " << prob << END_LOG;
                            } else {
                                //Could not compute the probability for
                                //the given level, so backing off (recursive)!

                                LOG_DEBUG << "Unable to find the " << SSTR(level)
                                        << "-Gram  prob for a (wordId,ctxId) = ("
                                        << wordId << ", " << ctxId
                                        << "), need to back off!" << END_LOG;

                                ATrie<N>::get_back_off_prob(ctxLen, prob);
                            }
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N

                            //The context length plus one is M value of the M-Gram
                            const TProbBackOffEntry * pEntry;
                            if (get_M_GramDataRef(level, wordId, ctxId, &pEntry)) {

                                LOG_DEBUG << "The " << level
                                        << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                        << "( prob. ) for (word,context) = ("
                                        << wordId << ", " << ctxId
                                        << "), is: " << pEntry->prob << END_LOG;

                                //Return the stored probability
                                prob = pEntry->prob;
                            } else {
                                //Could not compute the probability for
                                //the given level, so backing off (recursive)!

                                LOG_DEBUG << "Unable to find the " << SSTR(level)
                                        << "-Gram  prob for a (wordId,ctxId) = ("
                                        << wordId << ", " << ctxId
                                        << "), need to back off!" << END_LOG;

                                ATrie<N>::get_back_off_prob(ctxLen, prob);
                            }
                        }
                    } else {
                        //Could not compute the probability for
                        //the given level, so backing off (recursive)!

                        LOG_DEBUG << "Unable to find the " << SSTR(level)
                                << "-Gram  prob for a (wordId,ctxId) = ("
                                << wordId << ", " << ctxId
                                << "), need to back off!" << END_LOG;

                        ATrie<N>::get_back_off_prob(ctxLen, prob);
                    }
                } else {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    const TProbBackOffEntry * pEntry;
                    if (get_1_GramDataRef(wordId, & pEntry)) {

                        LOG_DEBUG << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( prob. ) for word: " << wordId
                                << ", is: " << pEntry->prob << END_LOG;

                        //Return the stored probability
                        prob = pEntry->prob;
                    } else {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << wordId << " returning:" << ZERO_LOG_PROB_WEIGHT
                                << " for log_" << LOG_PROB_WEIGHT_BASE << "( prob. )" << END_LOG;

                        //Return the default minimal probability for an unknown word
                        prob = ZERO_LOG_PROB_WEIGHT;
                    }
                }
            }

            template<TModelLevel N>
            bool ALayeredTrie<N>::get_prob_weight(const TModelLevel level, TLogProbBackOff & prob) {
                //Compute the context length of the given M-Gram
                const TModelLevel ctxLen = level - 1;
                //Get the last word in the N-gram
                const TShortId & wordId = ATrie<N>::get_end_word_id();

                LOG_DEBUG << "Computing probability for an " << level
                        << "-gram the context length is " << ctxLen << END_LOG;

                //Consider different variants based no the length of the context
                if (ctxLen > 0) {
                    //If we are looking for a M-Gram probability with M > 0, so not for a 1-Gram
                    TLongId ctxId;

                    //Compute the context id based on what is stored in m_GramWordIds and context length
                    if (get_query_context_Id<false>(ctxLen, ctxId)) {
                        LOG_DEBUG << "Got query context id: " << ctxId << END_LOG;
                        if (level == N) {
                            //If we are looking for a N-Gram probability
                            TLogProbBackOff n_gram_prob = ZERO_PROB_WEIGHT;
                            if (get_N_GramProb(wordId, ctxId, n_gram_prob)) {
                                LOG_DEBUG2 << "The " << N << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                        << "( prob. ) for (wordId,ctxId) = (" << wordId << ", "
                                        << ctxId << "), is: " << prob << END_LOG;
                                prob = n_gram_prob;
                                return true;
                            } else {
                                //Could not compute the probability for
                                //the given level, so backing off (recursive)!
                                LOG_DEBUG << "Unable to find the " << SSTR(level)
                                        << "-Gram  prob for a (wordId,ctxId) = ("
                                        << wordId << ", " << ctxId
                                        << "), need to back off!" << END_LOG;
                                return false;
                            }
                        } else {
                            //If we are looking for a M-Gram probability with 1 < M < N
                            //The context length plus one is M value of the M-Gram
                            const TProbBackOffEntry * pEntry;
                            if (get_M_GramDataRef(level, wordId, ctxId, &pEntry)) {
                                LOG_DEBUG << "The " << level
                                        << "-Gram log_" << LOG_PROB_WEIGHT_BASE
                                        << "( prob. ) for (word,context) = ("
                                        << wordId << ", " << ctxId
                                        << "), is: " << pEntry->prob << END_LOG;
                                //Return the stored probability
                                prob = pEntry->prob;
                                return true;
                            } else {
                                //Could not compute the probability for
                                //the given level, so backing off (recursive)!
                                LOG_DEBUG << "Unable to find the " << SSTR(level)
                                        << "-Gram  prob for a (wordId,ctxId) = ("
                                        << wordId << ", " << ctxId
                                        << "), need to back off!" << END_LOG;
                                return false;
                            }
                        }
                    } else {
                        //Could not compute the probability for
                        //the given level, so backing off (recursive)!
                        LOG_DEBUG << "Unable to find the " << SSTR(level)
                                << "-Gram  prob for a (wordId,ctxId) = ("
                                << wordId << ", " << ctxId
                                << "), need to back off!" << END_LOG;
                        return false;
                    }
                } else {
                    //If we are looking for a 1-Gram probability, no need to compute the context
                    const TProbBackOffEntry * pEntry;
                    if (get_1_GramDataRef(wordId, & pEntry)) {

                        LOG_DEBUG << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( prob. ) for word: " << wordId
                                << ", is: " << pEntry->prob << END_LOG;

                        //Return the stored probability
                        prob = pEntry->prob;
                        return true;
                    } else {
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << wordId << " returning:" << ZERO_LOG_PROB_WEIGHT
                                << " for log_" << LOG_PROB_WEIGHT_BASE << "( prob. )" << END_LOG;

                        //Return the default minimal probability for an unknown word
                        prob = ZERO_LOG_PROB_WEIGHT;
                        return true;
                    }
                }
            }

            template<TModelLevel N>
            void ALayeredTrie<N>::add_back_off_weight(const TModelLevel level, TLogProbBackOff & prob) {
                //Get the word hash for the en word of the back-off N-Gram
                const TShortId & wordId = ATrie<N>::get_back_off_end_word_id();
                const TModelLevel boCtxLen = level - 1;

                LOG_DEBUG << "Computing back-off for an " << level
                        << "-gram the context length is " << boCtxLen << END_LOG;

                if (boCtxLen > 0) {
                    //Attempt to retrieve back-off weights
                    TLongId ctxId = AWordIndex::UNDEFINED_WORD_ID;
                    //Compute the context hash
                    if (get_query_context_Id<true>(boCtxLen, ctxId)) {
                        LOG_DEBUG << "Got query context id: " << ctxId << END_LOG;
                        //The context length plus one is M value of the M-Gram
                        const TProbBackOffEntry * pEntry;
                        if (get_M_GramDataRef(level, wordId, ctxId, &pEntry)) {
                            //Obtained the stored back-off weight
                            prob += pEntry->back_off;
                            LOG_DEBUG << "The " << level << "-Gram log_"
                                    << LOG_PROB_WEIGHT_BASE << "( back-off ) for (wordId, ctxId)=("
                                    << wordId << ", " << ctxId << "), is: " << pEntry->back_off << END_LOG;
                        } else {
                            //The query context id could be determined, but 
                            //the data was not found in the trie.
                            LOG_DEBUG << "Unable to find data for " << (level)
                                    << "-Gram query with end wordId: "
                                    << SSTR(wordId) << ", ctxId: "
                                    << SSTR(ctxId) << "!" << END_LOG;
                        }
                    } else {
                        //The query context id could not be determined,
                        //so the M-gram is not present!
                        LOG_DEBUG << "Unable to find ctxId for " << (level)
                                << "-Gram query with end wordId: "
                                << SSTR(wordId) << "!" << END_LOG;
                    }
                } else {
                    //We came to a zero context, which means we have an
                    //1-Gram to try to get the back-off weight from
                    //Attempt to retrieve back-off weights
                    const TProbBackOffEntry * pbData;
                    if (get_1_GramDataRef(wordId, &pbData)) {
                        //Note that: If the stored back-off is UNDEFINED_LOG_PROB_WEIGHT then the back of is just zero
                        prob += pbData->back_off;
                        LOG_DEBUG << "The 1-Gram log_" << LOG_PROB_WEIGHT_BASE
                                << "( back-off ) for word: " << wordId
                                << ", is: " << pbData->back_off << END_LOG;
                    } else {
                        //The one gram data is not present!
                        LOG_DEBUG << "Unable to find the 1-Gram entry for a word: "
                                << wordId << ", nowhere to back-off!" << END_LOG;
                    }
                }
            }

            //Make sure that there will be templates instantiated, at least for the given parameter values
            template class ALayeredTrie<M_GRAM_LEVEL_MAX>;
        }
    }
}

