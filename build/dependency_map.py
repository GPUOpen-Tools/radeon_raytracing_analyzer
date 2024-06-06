#! python3
##=============================================================================
## Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
## \author AMD Developer Tools Team
## \file
## \brief List of all external dependencies.
##=============================================================================

import sys

# prevent generation of .pyc file
sys.dont_write_bytecode = True

####### Git Dependencies #######

# To allow for future updates where we may have cloned the project, store the root of
# the repo in a variable. In future, we can automatically calculate this based on the git config
github_tools = "https://github.com/GPUOpen-Tools/"
github_root  = "https://github.com/"

# Define a set of dependencies that exist as separate git projects.
# each git dependency has a desired directory where it will be cloned - along with a commit to checkout
# The third parameter in the value field is whether to do a shallow clone. Usually, this will be True but if a commit hash is used as a branch, a full clone is needed.
git_mapping = {
    github_tools + "qt_common"                                      : ["../external/qt_common",          "v4.0.0",                                   True],
    github_tools + "update_check_api"                               : ["../external/update_check_api",   "v2.1.1",                                   True],
    github_tools + "system_info_utils"                              : ["../external/system_info_utils",  "88a338a01949f8d8bad60a30b78b65300fd13a9f", False],
    github_root  + "g-truc/glm"                                     : ["../external/third_party/glm",    "0.9.9.8",                                  True],
    github_root  + "KhronosGroup/Vulkan-Headers"                    : ["../external/third_party/vulkan", "sdk-1.3.211",                              True],
    github_root  + "zeux/volk"                                      : ["../external/third_party/volk",   "1.2.190",                                  True],
    github_root  + "GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator" : ["../external/vma",                "d2f0313d20c803f83cc3637ac1facf8e4d6899e4", False],
    github_root  + "GPUOpen-Drivers/libamdrdf"                      : ["../external/rdf",                "v1.1.2",                                   True],
}
