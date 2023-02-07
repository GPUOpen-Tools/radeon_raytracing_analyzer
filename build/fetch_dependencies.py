#! python3
# Copyright (c) 2020-2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Script to fetch all external git and/or downloadable dependencies needed to build the project
#
#   fetch_dependencies.py
#
# Each git repo will be updated to the commit specified in the "gitMapping" table.

import os
import subprocess
import sys
import zipfile
import tarfile
import platform
import argparse

# Check for the python 3.x name and import it as the 2.x name
try:
    import urllib.request as urllib
# if that failed, then try the 2.x name
except ImportError:
    import urllib

# to allow the script to be run from anywhere - not just the cwd - store the absolute path to the script file
script_root = os.path.dirname(os.path.realpath(__file__))

# also store the basename of the file
script_name = os.path.basename(__file__)

# Print a message to the console with appropriate pre-amble
def log_print(message):
    print ("\n" + script_name + ": " + message)
    sys.stdout.flush()

# add script root to support import of URL and git maps
sys.path.append(script_root)
from dependency_map import git_mapping

# Clone or update a git repo
def update_git_dependencies(git_mapping, update):
    for git_repo in git_mapping:
        # add script directory to path
        tmp_path = os.path.join(script_root, git_mapping[git_repo][0])

        # clean up path, collapsing any ../ and converting / to \ for Windows
        path = os.path.normpath(tmp_path)

        # required commit
        reqd_commit = git_mapping[git_repo][1]

        do_checkout = False
        if not os.path.isdir(path):
            # directory doesn't exist - clone from git
            log_print("Directory %s does not exist, using 'git clone' to get latest from %s" % (path, git_repo))
            p = subprocess.Popen((["git", "clone", git_repo ,path]), stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode == 0):
                do_checkout = True
            else:
                log_print("git clone failed with return code: %d" % p.returncode)
                return False
        elif update == True:
            # directory exists and update requested - get latest from git
            log_print("Directory %s exists, using 'git fetch --tags -f' to get latest from %s" % (path, git_repo))
            p = subprocess.Popen((["git", "fetch", "--tags", "-f"]), cwd=path, stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode == 0):
                do_checkout = True
            else:
                log_print("git fetch failed with return code: %d" % p.returncode)
                return False
        else:
            # Directory exists and update not requested
            log_print("Git Dependency %s found and not updated" % git_repo)

        if do_checkout == True:
            log_print("Checking out required commit: %s" % reqd_commit)
            p = subprocess.Popen((["git", "checkout", reqd_commit]), cwd=path, stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode != 0):
                log_print("git checkout failed with return code: %d" % p.returncode)
                return False
            log_print("Ensuring any branch is on the head using git pull --ff-only origin %s" % reqd_commit)
            p = subprocess.Popen((["git", "pull", "--ff-only", "origin", reqd_commit]), cwd=path, stderr=subprocess.STDOUT)
            p.wait()
            if(p.returncode != 0):
                log_print("git merge failed with return code: %d" % p.returncode)
                return False

    return True

# Main body of update functionality
def do_fetch_dependencies(update):
    # Print git version being used
    git_cmd = ["git", "--version"]
    git_output = subprocess.check_output(git_cmd, stderr=subprocess.STDOUT)
    log_print("%s" % git_output)

    # Update all git dependencies
    if update_git_dependencies(git_mapping, update):
        return True
    else:
        return False

if __name__ == '__main__':
    # fetch_dependencies.py executed as a script

    # parse the command line arguments
    parser = argparse.ArgumentParser(description="A script that fetches all the necessary build dependencies for the project")
    args = parser.parse_args()

    do_fetch_dependencies(True)
