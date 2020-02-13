#!/usr/bin/env python

'''
  Copyright (c) 2019-2020 Valve Corporation
  Copyright (c) 2019-2020 LunarG, Inc.
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
      http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 
  Author: Jeremy Kniager <jeremyk@lunarg.com>
 
 '''

from __future__ import print_function

import argparse
import datetime
import json
import os
import stat
import sys
import shutil

def main():
    # Parse commandline arguments
    parser = argparse.ArgumentParser(
        description='Install Vulkan loader, layers, and MoltenVK ICD')
    parser.add_argument(
        '--install-json-location',
        dest='install_json_location',
        default='.',
        help="Location of INSTALL_BOM.json. Default is \'.\'."
    )
    parser.add_argument(
        '--log-file',
        dest='log_file',
        default='INSTALL.log',
        help="Name of log file. Default is \'INSTALL.log\'."
    )
    parser.add_argument(
        '--force-install',
        action='store_true',
        dest='force_install',
        help="Force install of SDK files."
    )
    args = parser.parse_args()

    install_bom_json = os.path.join(os.path.abspath(args.install_json_location), "INSTALL_BOM.json")
    uninstall_filename = "uninstall.sh"
    log_file = args.log_file
    force_install = args.force_install

    # Load INSTALL_BOM.json as a dictionary.
    # INSTALL_BOM.json contains a list of files/directories to be copied from the SDK to system directories.
    # Dictionary keys are relative paths to the files/directories to be installed.
    # Dictionary Items:
    #    Dest: Absolute path to system directory the file/directory will be copied to
    #    Dir: Boolean value for whether the key is a directory.
    files_to_copy = {}
    with open(install_bom_json) as install_bom:
        file_contents = install_bom.read()
        files_to_copy = json.loads(file_contents)
    
    # Convert the relative paths in 'files_to_copy' to absolute paths.
    absolute_copy_files = {}
    for key in files_to_copy:
        abspath_key = os.path.abspath(key)
        if files_to_copy[key]["Dir"]:
            absolute_copy_files[abspath_key] = {
                "Dest" : os.path.join(os.path.abspath(files_to_copy[key]["Dest"]), os.path.basename(key)),
                "Link" : files_to_copy[key]["Link"],
                "Dir" : files_to_copy[key]["Dir"]
            }
        else:
            absolute_copy_files[abspath_key] = {
                "Dest" : os.path.abspath(files_to_copy[key]["Dest"]),
                "Link" : files_to_copy[key]["Link"],
                "Dir" : files_to_copy[key]["Dir"]
            }

    # Check if SDK files/directories are already installed.
    files_found = []
    for key in absolute_copy_files:
        if absolute_copy_files[key]["Dir"]:
            file_to_check = absolute_copy_files[key]["Dest"]
        else:
            file_to_check = os.path.join(absolute_copy_files[key]["Dest"], os.path.split(key)[1])
        if os.path.exists(file_to_check):
            files_found.append(file_to_check)

    if len(files_found) > 0:
        print("The following files were found to already be installed.")
        print("Left over from past install?")
        for f in files_found:
            print("\t" + str(f))
        if not force_install:
            print("Stopping install.  If you want to install anyway, run with argument '--force-install'.")
            exit()

    # Generate uninstall script to remove Vulkan SDK files from system directories.
    with open(uninstall_filename, "w") as uninstall_file:
        uninstall_file.write("""
        if [ ! $(uname) == "Darwin" ]; then
            echo "This script should be run on MacOS ONLY."
            echo "It is meant to uninstall the Vulkan SDK on MacOS."
            exit 1
        fi
        
        echo >> {0}
        date >> {0}
        """.format(log_file))
        for key in files_to_copy:
            remove_file_path = os.path.join(files_to_copy[key]["Dest"], os.path.basename(key))
            uninstall_file.write("""
            echo "Removing {0}" | tee -a {1}
            rm -rf {0}
            """.format(remove_file_path, log_file))

    os.chmod(uninstall_filename, stat.S_IRWXU | stat.S_IRGRP | stat.S_IXGRP | stat.S_IROTH | stat.S_IXOTH)

    # Write what the program is doing to a log file.
    with open(log_file, 'a') as lf:
        lf.write('\n')
        lf.write(datetime.datetime.now().strftime('%c') + '\n')
        for key in absolute_copy_files:
            log_str = str("Copying " + str(key) + " to " + str(absolute_copy_files[key]["Dest"]))
            print(log_str)
            lf.write(log_str + '\n')
            if not absolute_copy_files[key]["Dir"] and not os.path.isdir(absolute_copy_files[key]["Dest"]):
                os.mkdir(absolute_copy_files[key]["Dest"])
            if absolute_copy_files[key]["Link"] is not None:
                file_to_check = os.path.join(absolute_copy_files[key]["Dest"], os.path.split(key)[1])
                if force_install and file_to_check in files_found:
                    if absolute_copy_files[key]["Dir"]:
                        shutil.rmtree(file_to_check)
                    else:
                        os.remove(file_to_check)
                if sys.version_info[0] == 2:
                    os.symlink(absolute_copy_files[key]["Link"], os.path.join(absolute_copy_files[key]["Dest"], os.path.basename(key)))
                else:
                    os.symlink(absolute_copy_files[key]["Link"], os.path.join(absolute_copy_files[key]["Dest"], os.path.basename(key)), target_is_directory=absolute_copy_files[key]["Dir"])
            elif absolute_copy_files[key]["Dir"]:
                if force_install and absolute_copy_files[key]["Dest"] in files_found:
                    shutil.rmtree(absolute_copy_files[key]["Dest"])
                shutil.copytree(key, absolute_copy_files[key]["Dest"], symlinks=True)
            else:
                shutil.copy(key, absolute_copy_files[key]["Dest"])

if __name__ == '__main__':
    main()
