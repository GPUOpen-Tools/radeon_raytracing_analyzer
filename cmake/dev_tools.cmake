#######################################################################################################################
### Copyright (c) 2019-2023 Advanced Micro Devices, Inc. All rights reserved.
### \author AMD Developer Tools Team
#######################################################################################################################

cmake_minimum_required(VERSION 3.10)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    string(REPLACE " /W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()

# Apply options to a developer tools target.
# These options are hard requirements to build. If they cannot be applied, we
# will need to fix the offending target to ensure it complies.
function(devtools_target_options name)

    set_target_properties(${name} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON)

    get_target_property(target_type ${name} TYPE)
    if (${target_type} STREQUAL "INTERFACE_LIBRARY")
        return()
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")

        target_compile_options(${name}
                PRIVATE
                -Wall
                -Werror
                -Wextra
                )

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name}
                PRIVATE
                /W4
                /WX
                /MP

                # disable warning C4201: nonstandard extension used: nameless struct/union
                /wd4201

                # TODO this warning is caused by the QT header files - use pragma to disable at source
                # disable warning C4127: conditional expression is constant
                /wd4127

                # Disable warnings about deprecated features
                # This happens when using later versions of Qt than RRA defaults to.
                /wd4996

                # this warning is caused by QT header files and has been introduced by VS2019 16.9.6
                # disable warning C5240: 'nodiscard': attribute is ignored in this syntactic position
                /wd5240
                )
    else ()

        message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not supported!")

    endif ()

    # GNU specific flags
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${name} PRIVATE -Wno-maybe-uninitialized)
    endif ()

    if (UNIX AND NOT APPLE)
        target_compile_definitions(${name}
                PRIVATE
                _LINUX

                # Use _DEBUG on Unix for Debug Builds (defined automatically on Windows)
                $<$<CONFIG:Debug>:_DEBUG>
                )
    endif ()

endfunction()
