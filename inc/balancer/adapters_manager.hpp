/* 
 * File:   adapters_manager.hpp
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
 * Created on July 7, 2016, 12:09 PM
 */

#ifndef ADAPTERS_MANAGER_HPP
#define ADAPTERS_MANAGER_HPP

#include <set>
#include <map>
#include <vector>
#include <random>
#include <unordered_map>
#include <algorithm>
#include <functional>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"
#include "common/utils/threads/threads.hpp"
#include "common/utils/text/string_utils.hpp"

#include "common/messaging/language_registry.hpp"
#include "client/messaging/supp_lang_resp_in.hpp"
#include "server/messaging/supp_lang_resp_out.hpp"

#include "balancer/balancer_consts.hpp"
#include "balancer/balancer_parameters.hpp"
#include "balancer/translator_adapter.hpp"

using namespace std;

using namespace uva::utils::logging;
using namespace uva::utils::exceptions;
using namespace uva::utils::threads;
using namespace uva::utils::text;

using namespace uva::smt::bpbd::common::messaging;
using namespace uva::smt::bpbd::client::messaging;
using namespace uva::smt::bpbd::server::messaging;

namespace uva {
    namespace smt {
        namespace bpbd {
            namespace balancer {

                /**
                 * This is the translation servers manager class:
                 * Responsibilities:
                 *      Keeps track of the online translation servers
                 *      Keeps track of languages supported by the servers
                 *      Keeps track of the known load on the servers
                 *      Advises translation server for a translation request
                 */
                class adapters_manager {
                public:

                    //Typedef for the set storing the pointers to the adapters
                    typedef vector<translator_adapter*> adapters_list;

                    /**
                     * This structure represents the target language entry
                     * in the map of target to adapter vectors
                     */
                    typedef struct {
                        //The mutex for the targets map of the source language
                        shared_mutex m_adapters_mutex;
                        //The list of the dapters
                        adapters_list m_adapters;
                        //Stores the random number generator for the target entry
                        discrete_distribution<uint32_t> m_distribution;

                        /**
                         * Allows to get the translation server adapter to do the next translation.
                         * This function synchronizes the retrieval on the adapters mutex.
                         * @return the translation server adapter to do the next translation
                         *         or NULL if the there is no adapters available.
                         */
                        inline translator_adapter* get_adapter() {
                            //Add the adapter to the list of adapters
                            {
                                shared_guard guard(m_adapters_mutex);
                                switch (m_adapters.size()) {
                                    case 0:
                                        return NULL;
                                    case 1:
                                        return m_adapters[0];
                                    default:
                                        return m_adapters[m_distribution(m_generator)];
                                }
                            }
                        }

                    private:
                        //Stores the random engine generator to be used
                        default_random_engine m_generator;
                    } target_entry;

                    /**
                     * This class represents an adapter entry storing the
                     * translation server adapter and supplementary mapping
                     * information.
                     */
                    typedef struct {
                        //Stores the adapter
                        translator_adapter m_adapter;
                        //Stores the mutex for accessing the registrations
                        shared_mutex m_registrations_mutex;
                        //Stores the set of target entries in which this adaper is used.
                        set<target_entry*> m_registrations;
                    } adapter_entry;

                    //The typedef for the map strong the adapters
                    typedef map<string, adapter_entry> adapters_map;

                    //The typedef for the map for storing target language to target entry mapping;
                    typedef unordered_map<language_uid, target_entry> targets_map;

                    /**
                     * This structure represents the source language entry
                     * in the map of source to target language adapters
                     */
                    typedef struct {
                        //The mutex for the targets map of the source language
                        shared_mutex m_target_mutex;
                        //The targets to targets' entry map
                        targets_map m_targets;
                    } source_entry;
                    //The typedef for the map for storing source language to source entry mapping
                    typedef unordered_map<language_uid, source_entry> sources_map;

                    /**
                     * The basic constructor
                     * @param params the balancer parameters to configure from
                     */
                    adapters_manager(const balancer_parameters & params,
                            trans_resp_notifier trans_resp_func,
                            adapter_disc_notifier adapter_disc_func)
                    : m_params(params), m_adapters_data(), m_re_connect(NULL),
                    m_re_connect_mutex(), m_re_connect_condition(),
                    m_is_reconnect_run(true), m_source_mutex(), m_sources(), m_supp_lan_resp(),
                    m_supp_lan_resp_str(""), m_supp_lang_mutex() {
                        LOG_INFO3 << "Configuring the translation servers' manager" << END_LOG;

                        //Pre-create the notifier function types
                        ready_conn_notifier_type ready_conn_func = bind(&adapters_manager::notify_adapter_ready, this, _1, _2);
                        closed_conn_notifier_type closed_conn_func = bind(&adapters_manager::notify_disconnected, this, _1);

                        //Iterate through the list of translation server
                        //configs and create an adapter for each of them
                        for (auto iter = m_params.trans_servers.begin(); iter != m_params.trans_servers.end(); ++iter) {
                            LOG_INFO3 << "Configuring '" << iter->second.m_name << "' adapter..." << END_LOG;
                            m_adapters_data[iter->first].m_adapter.configure(iter->second,
                                    trans_resp_func, adapter_disc_func, ready_conn_func, closed_conn_func);
                            LOG_INFO2 << "'" << iter->second.m_name << "' adapter is configured" << END_LOG;
                        }

                        LOG_INFO2 << "The translation servers are configured" << END_LOG;
                    }

                    /**
                     * The main method to enable the translation servers manager
                     */
                    inline void enable() {
                        LOG_DEBUG << "Enabling the translation servers' manager" << END_LOG;

                        //First begin the reconnection thread
                        start_re_connection_thread();

                        //Enable the clients
                        for (auto iter = m_adapters_data.begin(); iter != m_adapters_data.end(); ++iter) {
                            LOG_DEBUG1 << "Enabling '" << iter->second.m_adapter.get_name() << "' adapter..." << END_LOG;
                            iter->second.m_adapter.enable();
                            LOG_DEBUG1 << "'" << iter->second.m_adapter.get_name() << "' adapter is enabled" << END_LOG;
                        }

                        LOG_DEBUG << "The translation servers are enabled" << END_LOG;
                    }

                    /**
                     * Allows to disable the translation servers manager
                     */
                    inline void disable() {
                        LOG_USAGE << "Stopping the translation server clients ..." << END_LOG;

                        //First remove the reconnection thread
                        finish_re_connection_thread();

                        LOG_INFO2 << "Disabling the translation servers' manager" << END_LOG;

                        //Disable the clients
                        for (auto iter = m_adapters_data.begin(); iter != m_adapters_data.end(); ++iter) {
                            LOG_INFO2 << "Disabling '" << iter->second.m_adapter.get_name() << "' adapter..." << END_LOG;
                            iter->second.m_adapter.disable();
                            LOG_INFO1 << "'" << iter->second.m_adapter.get_name() << "' adapter is disabled" << END_LOG;
                        }

                        LOG_INFO1 << "The translation servers are disabled" << END_LOG;
                    }

                    /**
                     * Reports the run-time information
                     */
                    inline void report_run_time_info() {
                        LOG_USAGE << "Translation servers (#" << m_adapters_data.size() << "): " << END_LOG;
                        for (auto iter = m_adapters_data.begin(); iter != m_adapters_data.end(); ++iter) {
                            iter->second.m_adapter.report_run_time_info();
                        }
                    }

                    /**
                     * Allows to request a translation servers' manager for the
                     * translation server adapter given translation job request.
                     * @param trans_req the translation request, not NULL
                     * @return the pointer to the translation server dataper or NULL
                     * if there is no adapter for the given source/target language pair
                     */
                    inline translator_adapter * get_translator_adapter(const trans_job_req_in * trans_req) {
                        //Get the response source language id
                        const language_uid source_uid = trans_req->get_source_lang_uid();
                        //Get the response target language id
                        const language_uid target_uid = trans_req->get_target_lang_uid();

                        //Get/create a source language entry, note that the entries are
                        //not removed, until the translation servers' manager is destroyed.
                        //Therefore it is safe to store the pointer to the entry.
                        source_entry * source = NULL;
                        {
                            shared_guard guard(m_source_mutex);
                            //Get the source entry
                            source = &m_sources[source_uid];
                        }

                        //Get/create a target language entry, note that the entries are
                        //not removed, until the translation servers' manager is destroyed.
                        //Therefore it is safe to store the pointer to the entry.
                        target_entry * target = NULL;
                        {
                            shared_guard guard(source->m_target_mutex);
                            //Get the stored or new list of the adapters
                            target = &source->m_targets[target_uid];
                        }

                        //Return the advised adapter
                        return target->get_adapter();
                    }

                    /**
                     * Allows to get the string storing the serialized supported languages response
                     * @return the string storing the serialized supported languages response
                     */
                    inline string get_supported_lang_resp_data() {
                        shared_guard guard(m_supp_lang_mutex);

                        //Return the pre-computed string value
                        return m_supp_lan_resp_str;
                    }

                protected:

                    /**
                     * Allows to notify the manager that there is an adapter redy to receive translation requests
                     * @param adapter the pointer to the translation server adapter that got ready, not NULL
                     * @param lang_resp_msg the supported language pairs message, to be destroyed by this method
                     */
                    inline void notify_adapter_ready(translator_adapter * adapter, supp_lang_resp_in * lang_resp_msg) {
                        exclusive_guard guard(m_supp_lang_mutex);
                        LOG_DEBUG << "The server adapter '" << adapter->get_name() << "' is connected!" << END_LOG;

                        //Get the adapter to lock on its registrations
                        //Note: The adapters are not getting removed from the global
                        //adapters list for now so it is safe to keep the reference
                        //and not to synchronize on any mutex for the entire list of adapters.
                        adapter_entry & adapter_data = m_adapters_data[adapter->get_name()];
                        {
                            scoped_guard guard(adapter_data.m_registrations_mutex);

                            //Register the new ready adapter for the given set of translation pairs
                            const Value & languages = lang_resp_msg->get_languages();

                            //Iterate through the supported source and target languages
                            //to register them as supported by the given server adapter.
                            for (auto sli = languages.MemberBegin(); sli != languages.MemberEnd(); ++sli) {
                                const string source_lang = sli->name.GetString();
                                const language_uid source_uid = language_registry::register_uid(source_lang);
                                LOG_DEBUG1 << "'" << adapter->get_name() << "' source: " << source_lang
                                        << " uid: " << to_string(source_uid) << END_LOG;

                                //Iterate through the possible multiple targets
                                for (auto tli = sli->value.Begin(); tli != sli->value.End(); ++tli) {
                                    const string target_lang = tli->GetString();
                                    const language_uid target_uid = language_registry::register_uid(target_lang);
                                    LOG_DEBUG1 << "'" << adapter->get_name() << "'    target: " << target_lang
                                            << " uid: " << to_string(target_uid) << END_LOG;

                                    //Get/create a source language entry, note that the entries are
                                    //not removed, until the translation servers' manager is destroyed.
                                    //Therefore it is safe to store the pointer to the entry.
                                    source_entry * source = NULL;
                                    {
                                        exclusive_guard guard(m_source_mutex);
                                        //Get the source entry
                                        source = &m_sources[source_uid];
                                    }

                                    //Get/create a target language entry, note that the entries are
                                    //not removed, until the translation servers' manager is destroyed.
                                    //Therefore it is safe to store the pointer to the entry.
                                    target_entry * target = NULL;
                                    {
                                        exclusive_guard guard(source->m_target_mutex);
                                        //Get the stored or new list of the adapters
                                        target = &source->m_targets[target_uid];
                                    }

                                    //Add the adapter to the list of adapters
                                    {
                                        exclusive_guard guard(target->m_adapters_mutex);
                                        //Add the adapter pointer to the list
                                        target->m_adapters.push_back(adapter);
                                        //Re-calculate the loads balance
                                        re_calculate_loads(target);
                                    }

                                    //Remember the list in which the adapter was placed
                                    adapter_data.m_registrations.insert(target);
                                }
                            }
                        }

                        //Re-calculate the supported languages response
                        update_supp_lang_resp();

                        //Destroy the message as it is not needed any more
                        delete lang_resp_msg;
                    }

                    /**
                     * Allows to notify that there were disconnected servers.
                     * This is needed to make sure that 
                     * NOTE: This method does not notify the re-connection thread as otherwise we
                     *       would be re-connecting too often, just let it work on the time-out
                     * @param adapter the pointer to the translation server adapter that got disconnected, not NULL
                     */
                    inline void notify_disconnected(translator_adapter * adapter) {
                        exclusive_guard guard(m_supp_lang_mutex);
                        LOG_DEBUG << "The server adapter '" << adapter->get_name() << "' is disconnected!" << END_LOG;

                        //Get the adapter to lock on its registrations
                        //Note: The adapters are not getting removed from the global
                        //adapters list for now so it is safe to keep the reference
                        //and not to synchronize on any mutex for the entire list of adapters.
                        adapter_entry & adapter_data = m_adapters_data[adapter->get_name()];
                        {
                            scoped_guard guard(adapter_data.m_registrations_mutex);

                            //Make sure that the corresponding supported language pairs are removed.
                            for (auto data_iter = adapter_data.m_registrations.begin();
                                    data_iter != adapter_data.m_registrations.end(); ++data_iter) {
                                //Get the target pointer
                                target_entry * target = *data_iter;

                                //Lock on the adapters list for safe multi-threading
                                //Remove the adapter from the adapters list and re-calculate the loads
                                {
                                    exclusive_guard guard(target->m_adapters_mutex);
                                    //Get the reference to the set of adapters of the target entry
                                    adapters_list & adapters = target->m_adapters;
                                    //Find the adapter in the set of the target language adapters
                                    auto adapter_iter = find(adapters.begin(), adapters.end(), adapter);
                                    //Remove the found adapter from this set
                                    if (adapter_iter != adapters.end()) {
                                        adapters.erase(adapter_iter);
                                    }
                                    //Re-calculate the loads for the target
                                    re_calculate_loads(target);
                                }
                            }
                            //Erase all the adapter registrations
                            adapter_data.m_registrations.clear();
                        }

                        //Re-calculate the supported languages response
                        update_supp_lang_resp();
                    }

                private:
                    //Stores the reference to the parameters structure 
                    const balancer_parameters & m_params;
                    //Stores the mapping from the server names to the server adapters
                    adapters_map m_adapters_data;
                    //Stores the pointer to the re-connection thread
                    thread * m_re_connect;
                    //Stores the synchronization primitive instances
                    mutex m_re_connect_mutex;
                    condition_variable m_re_connect_condition;
                    //Stores the flag that indicates for how long the reconnection thread needs to run
                    a_bool_flag m_is_reconnect_run;
                    //Stores the synchronization mutex for the manager
                    shared_mutex m_source_mutex;
                    //Stores the language pair mappings to the adapters map
                    sources_map m_sources;
                    //Stores the supported languages response
                    supp_lang_resp_out m_supp_lan_resp;
                    //Stores the supported languages response
                    string m_supp_lan_resp_str;
                    //The mutex to synchronize generation of the supported language responses
                    shared_mutex m_supp_lang_mutex;

                    /**
                     * This function allows to update the supported languages response
                     * and must be called from a sychronized context that ensures that
                     * there are no changes made in between to the source or target entries
                     * I.e. the supported languages stay the same until the new response is
                     * fully build and generated.
                     */
                    inline void update_supp_lang_resp() {
                        //Re-set the response
                        m_supp_lan_resp.reset();

                        //Start the object
                        m_supp_lan_resp.start_supp_lang_obj();

                        //Iterate through the languages and build the response
                        //No need to synchronize here as we must be in a
                        //thread safe caller's context.
                        for (auto s_iter = m_sources.begin(); s_iter != m_sources.end(); ++s_iter) {
                            //Get the targets
                            targets_map & targets = s_iter->second.m_targets;
                            //Check if there are currently known targets
                            if (targets.size() > 0) {
                                //Start the array
                                m_supp_lan_resp.start_source_lang_arr(language_registry::get_name(s_iter->first));
                                for (auto t_iter = targets.begin(); t_iter != targets.end(); ++t_iter) {
                                    adapters_list & adapters = t_iter->second.m_adapters;
                                    //Check if the target has ready adapters
                                    if (adapters.size() > 0) {
                                        m_supp_lan_resp.add_target_lang(language_registry::get_name(t_iter->first));
                                    }
                                }
                                //End the array
                                m_supp_lan_resp.end_source_lang_arr();
                            }
                        }

                        //End the object
                        m_supp_lan_resp.end_supp_lang_obj();

                        //Serialize and store the string
                        m_supp_lan_resp_str = m_supp_lan_resp.serialize();

                        LOG_DEBUG << "Supported languages response: " << m_supp_lan_resp_str << END_LOG;
                    }

                    /**
                     * Allows to re-calculate the loads for the target entry.
                     * Should be called when a new adapter was added/removed.
                     * Not synchronized, is to be called from a context with
                     * an exclusive lock on target->m_adapters_mutex
                     * @param target the pointer to the target entry, not NULL
                     */
                    inline void re_calculate_loads(target_entry * target) {
                        //Initialize an array of weights
                        vector<uint32_t> weights;

                        //Fill in the vector
                        for (auto iter = target->m_adapters.begin(); iter != target->m_adapters.end(); ++iter) {
                            weights.push_back((*iter)->get_weight());
                        }

                        //Re-set the distribution
                        discrete_distribution<uint32_t> new_distribution(weights.begin(), weights.end());

                        //Assign the new distribution to the stored one
                        target->m_distribution = new_distribution;

                        LOG_DEBUG << "The new target weights: " << vector_to_string(weights) << END_LOG;
                    }

                    /**
                     * Is run within a separate thread which allows to periodically
                     * try to re-connect disconnected the servers.
                     */
                    inline void re_connect_servers() {
                        //Get the time to wait 
                        std::chrono::milliseconds time_to_wait = std::chrono::milliseconds{m_params.m_recon_time_out};

                        //Run the re-connection loop
                        while (m_is_reconnect_run) {
                            unique_guard guard(m_re_connect_mutex);

                            //Make the thread sleep, use the real server specific parameter in milliseconds
                            m_re_connect_condition.wait_for(guard, time_to_wait);

                            //If we were not waken up to stop, then check on the disconnected servers
                            if (m_is_reconnect_run) {
                                //Iterate through all the connectors and check for their activity
                                for (auto iter = m_adapters_data.begin(); iter != m_adapters_data.end(); ++iter) {
                                    iter->second.m_adapter.reconnect();
                                }
                            }
                        }
                    }

                    /**
                     * Allows to start the re-connection thread.
                     */
                    inline void start_re_connection_thread() {
                        //Check if the re-connection thread is present, if not then add it
                        if (m_re_connect == NULL) {
                            //Create a thread that will take care of re-connecting
                            m_re_connect = new thread(bind(&adapters_manager::re_connect_servers, this));
                        }
                    }

                    /**
                     * Allows to finish the re-connection thread.
                     */
                    inline void finish_re_connection_thread() {
                        if (m_re_connect != NULL) {
                            //Make sure that the re-connection thread terminates
                            m_is_reconnect_run = false;

                            //Wake up the thread
                            m_re_connect_condition.notify_all();

                            //Check if the re-connection thread is joinable
                            if (m_re_connect->joinable()) {
                                LOG_INFO << "Waiting for the re-connection thread to stop " << END_LOG;
                                m_re_connect->join();
                            }

                            //Delete the thread
                            delete m_re_connect;
                            m_re_connect = NULL;
                        }
                    }
                };
            }
        }
    }
}
#endif /* TRANSLATION_SERVERS_MANAGER_HPP */

