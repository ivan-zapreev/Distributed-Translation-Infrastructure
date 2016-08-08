/* 
 * File:   multi_threading.hpp
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
 * Created on January 28, 2016, 2:11 PM
 */

#ifndef MULTI_THREADING_HPP
#define MULTI_THREADING_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <ctime>

using namespace std;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

namespace uva {
    namespace utils {
        namespace threads {

            //Define the lock type to synchronize map operations
            typedef lock_guard<recursive_mutex> recursive_guard;

            //Define the lock type to synchronize map operations
            typedef lock_guard<mutex> scoped_guard;

            //Define the unique lock needed for wait/notify
            typedef unique_lock<mutex> unique_guard;

            //In C++11 which we use now there is no shared lock and
            //mutex, these are to be introduced in C++14/C++17. So
            //for now there is just an aliasing we have
#if _LIBCPP_STD_VER < 14
            using shared_mutex = mutex;
#else
#include <shared_mutex>
#endif
#if _LIBCPP_STD_VER < 17
            template<class _Mutex>
            using shared_lock = unique_lock<_Mutex>;
#endif

            //Define the exclusive lock needed for shared mutex
            typedef unique_lock<shared_mutex> exclusive_guard;

            //Define the shared lock needed for shared mutex
            typedef shared_lock<shared_mutex> shared_guard;

            //The atomic read-only boolean flag 
            typedef atomic<bool> a_bool_flag;

            //The atomic constant reference boolean flag 
            typedef const a_bool_flag & acr_bool_flag;
        }
    }
}

#endif /* MULTI_THREADING_HPP */

