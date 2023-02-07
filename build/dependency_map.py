#! python3
# Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.

import sys

# prevent generation of .pyc file
sys.dont_write_bytecode = True

####### Git Dependencies #######

# To allow for future updates where we may have cloned the project, store the root of
# the repo in a variable. In future, we can automatically calculate this based on the git config
github_tools  = "https://github.com/GPUOpen-Tools/"
github_root   = "https://github.com/"

# Define a set of dependencies that exist as separate git projects.
# each git dependency has a desired directory where it will be cloned - along with a commit to checkout
git_mapping = {
    github_tools + "QtCommon"                                       : ["../external/qt_common",          "v3.9.0"],
    github_tools + "UpdateCheckAPI"                                 : ["../external/update_check_api",   "v2.0.1"],
    github_root  + "g-truc/glm"                                     : ["../external/third_party/glm",    "0.9.9.8"],
    github_root  + "KhronosGroup/Vulkan-Headers"                    : ["../external/third_party/vulkan", "sdk-1.3.211"],
    github_root  + "zeux/volk"                                      : ["../external/third_party/volk",   "1.2.190"],
    github_root  + "GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator" : ["../external/vma",                "d2f0313d20c803f83cc3637ac1facf8e4d6899e4"],
    github_root  + "GPUOpen-Drivers/libamdrdf"                      : ["../external/rdf",                "v1.1.1"],
}
