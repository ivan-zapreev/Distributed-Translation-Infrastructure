/* 
 * File:   utils.hpp
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
 * Created on July 25, 2016, 3:25 PM
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdio.h>
#include <dirent.h>

#include "common/utils/exceptions.hpp"
#include "common/utils/logging/logger.hpp"

using namespace std;
using namespace uva::utils::logging;
using namespace uva::utils::exceptions;

namespace uva {
    namespace utils {
        namespace file {
            
            /**
             * Allows to check if the given folder exists, and if not then create one
             * Uses the system call so is not cross platform!
             * @param folder_name the name of the folder to check/create
             */
            static inline void check_create_folder(const string & folder_name) {
                //Open the specified folder
                DIR * pdir = opendir(folder_name.c_str());

                LOG_USAGE << "The lattice file folder is: " << folder_name << END_LOG;

                //Check if the directory is present
                if (pdir == NULL) {
                    //Log the warning that the directory does not exist
                    LOG_WARNING << "The directory: " << folder_name
                            << " does not exist, creating!" << END_LOG;

                    //Construct the command to create the directory
                    const string cmd = string("mkdir -p ") + folder_name;
                    //Try to create the directory recursively using shell
                    const int dir_err = system(cmd.c_str());

                    //Check that the directory has been created
                    ASSERT_CONDITION_THROW((-1 == dir_err),
                            string("Could not create the directory: ") +
                            folder_name);
                } else {
                    //Close the directory
                    closedir(pdir);
                }
            }
        }
    }
}

#endif /* UTILS_HPP */

