#######################################################################################################################
### Copyright (c) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
### @author AMD Developer Tools Team
#######################################################################################################################

cmake_minimum_required(VERSION 3.10)

# linuxdeployqt
if (UNIX AND NOT APPLE)
    include(FetchContent)
    FetchContent_Declare(
            linuxdeployqt
            URL "http://bdcartifactory.amd.com/artifactory/DevToolsBDC/Assets/radeon_developer_panel/qt6/linuxdeployqt.zip"
            SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/linuxdeployqt
            DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(linuxdeployqt)

    find_program(LINUXDEPLOYQT "linuxdeployqt" HINTS "${PROJECT_SOURCE_DIR}/external/linuxdeployqt")
    if (LINUXDEPLOYQT)
        message(STATUS "Found linuxdeployqt: ${LINUXDEPLOYQT}")
    else ()
        message(ERROR "linuxdeployqt not found but is required for build")
    endif ()
endif ()

# Attempt to automatically find Qt on the local machine
if (UNIX AND NOT APPLE)
    find_package(Qt6 QUIET COMPONENTS Core Widgets Network Gui Svg Test GuiPrivate CorePrivate)
else ()
    find_package(Qt6 QUIET COMPONENTS Core Widgets Network Gui Svg Test)
endif ()

if (Qt6_DIR)
    message(STATUS "Qt6 cmake package found at ${Qt6_DIR}")
endif ()

if (NOT Qt6_DIR)
    # Attempt to query Qt 5
    if (UNIX AND NOT APPLE)
        find_package(Qt5 QUIET COMPONENTS Core Widgets Network Gui Test X11Extras)
    else ()
        find_package(Qt5 QUIET COMPONENTS Core Widgets Network Gui Test)
    endif ()

    if (Qt5_DIR)
        message(STATUS "Qt5 cmake package found at ${Qt5_DIR}")
    else ()
        message(WARNING "Qt5 cmake package not found. Please specify Qt5_DIR manually or in the CMAKE_PREFIX_PATH")
    endif ()
endif ()


if (Qt5_DIR OR Qt6_DIR)
    #######################################################################################################################
    # Setup the INSTALL target to include Qt DLLs
    ##
    # Logic used to find and use Qt's internal deployment tool to copy Qt-based dependencies and DLLs upon building, so
    # that we can run the application from an IDE and so that we just have a simple build directory with all dependencies
    # already deployed that we can easily distribute
    #######################################################################################################################
    get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    if (WIN32)
        find_program(DEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    elseif (APPLE)
        find_program(DEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
    elseif (UNIX)
        find_program(QT_QMAKE_EXECUTABLE qmake HINTS "${_qt_bin_dir}")
        set(DEPLOYQT_EXECUTABLE ${LINUXDEPLOYQT})
    endif ()

    function(deploy_qt_build target include_mac output_directory)
        if (DEPLOYQT_EXECUTABLE)
            if (WIN32)
                if (Qt6_DIR)
                    set(DEPLOYQT_POST_BUILD_COMMAND
                            ${DEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target}> -verbose 0 --no-compiler-runtime --no-translations
                            WORKING_DIRECTORY ${output_directory})
                else ()
                    set(DEPLOYQT_POST_BUILD_COMMAND
                            ${DEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target}> -verbose 0 --no-compiler-runtime --no-translations --no-angle --no-system-d3d-compiler --no-opengl-sw
                            WORKING_DIRECTORY ${output_directory})
                endif ()
            elseif (UNIX AND NOT APPLE)
                set(DEPLOYQT_POST_BUILD_COMMAND
                        ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${EXTERNAL_DIR}/libtraceevent/lib:${EXTERNAL_DIR}/libtracefs/lib:${_qt_bin_dir}/../lib
                        ${DEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target}> -qmake=${QT_QMAKE_EXECUTABLE} -verbose=0 -unsupported-allow-new-glibc
                        WORKING_DIRECTORY ${output_directory})
            elseif (include_mac)

                set(DEPLOYQT_POST_BUILD_COMMAND
                        ${DEPLOYQT_EXECUTABLE} $<TARGET_FILE_NAME:${target}>.app -verbose=0 -no-strip
                        WORKING_DIRECTORY ${output_directory})
            endif ()

            # Deploy Qt to build directory after a successful build
            add_custom_command(
                    TARGET ${target} POST_BUILD
                    COMMAND ${DEPLOYQT_POST_BUILD_COMMAND}
            )
        endif ()
    endfunction()

    function(deploy_qt_install_hook target component)
        if (DEPLOYQT_EXECUTABLE)
            deploy_qt_build(${target} TRUE)

            set(target_file_dir "$<TARGET_FILE_DIR:${target}>")

            # Handle installation of Qt dependencies
            if (WIN32)
                # Debug dlls end with a `d.dll`
                set(qt_suffix "$<$<CONFIG:Debug>:d>.dll")

                # Due to windows requiring DLLs be shipped adjacent we must be explicit here...
                # TODO: Maybe eventually we could look into some sort of manifest file?
                if (Qt5_DIR)
                    install(FILES
                            ${target_file_dir}/Qt5Core${qt_suffix}
                            ${target_file_dir}/Qt5Gui${qt_suffix}
                            ${target_file_dir}/Qt5Svg${qt_suffix}
                            ${target_file_dir}/Qt5Widgets${qt_suffix}
                            DESTINATION . COMPONENT ${component})

                    install(FILES ${target_file_dir}/Qt5Network${qt_suffix} DESTINATION . COMPONENT ${component} OPTIONAL)
                else ()
                    install(FILES
                            ${target_file_dir}/Qt6Core${qt_suffix}
                            ${target_file_dir}/Qt6Gui${qt_suffix}
                            ${target_file_dir}/Qt6Svg${qt_suffix}
                            ${target_file_dir}/Qt6Widgets${qt_suffix}
                            DESTINATION . COMPONENT ${component})
                    install(FILES ${target_file_dir}/Qt6Network${qt_suffix} DESTINATION . COMPONENT ${component} OPTIONAL)

                endif ()

                install(DIRECTORY ${target_file_dir}/iconengines DESTINATION . COMPONENT ${component})
                install(DIRECTORY ${target_file_dir}/imageformats DESTINATION . COMPONENT ${component})
                install(DIRECTORY ${target_file_dir}/platforms DESTINATION . COMPONENT ${component})
                install(DIRECTORY ${target_file_dir}/styles DESTINATION . COMPONENT ${component})
            elseif (UNIX AND NOT APPLE)
                # This is only needed for Linux as Apple platforms deploy Qt into the app bundle
                install(FILES
                        ${target_file_dir}/qt.conf
                        DESTINATION . COMPONENT ${component})
                install(DIRECTORY ${target_file_dir}/lib DESTINATION . COMPONENT ${component})
                install(DIRECTORY ${target_file_dir}/plugins DESTINATION . COMPONENT ${component})
            endif ()

        else ()
            message(FATAL_ERROR "Qt deployment executable not found.")
        endif ()
    endfunction()

    # Apply Qt-project specific options
    # Currently, this is only used for an issue with Clang on Windows
    function(DevDriverApplyQtOptions target)
        # WA: On Windows with Clang, we need rtti to compile. - August 2019
        iF (WIN32 AND ${CMAKE_CXX_COMPILER_ID} MATCHES " Clang ")
            target_compile_options(${target} PUBLIC -frtti)
        endif ()
    endfunction()
endif ()
