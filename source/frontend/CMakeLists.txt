cmake_minimum_required(VERSION 3.11)
project(RadeonRaytracingAnalyzer)

# switch on the autogenerators needed for Qt. Includes the MetaObject Compiler (MOC),
# the UI compiler (UIC) and the resource compiler (RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add UpdateCheckAPI
add_subdirectory (${PROJECT_SOURCE_DIR}/../../external/update_check_api ${CMAKE_CURRENT_BINARY_DIR}/update_check_api)
include_directories(${UPDATECHECKAPI_INC_DIRS})

IF (WIN32)
    # get the location of the Visual Studio redist libraries - but don't create an install rule to install them
    # the libs will be copied manually as a post build step
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
ENDIF(WIN32)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(AFTER ../backend ../frontend ../renderer)
IF(UNIX)
    include_directories(AFTER ../backend/Linux)

    # Include the qt version subdirectory path.
    set(result "")
    set(QT_NATIVE_PATH "")
    foreach(path ${Qt5Gui_INCLUDE_DIRS})
        string(FIND ${path} "QtGui" result)
        if (NOT ${result} STREQUAL "-1")
            set(QT_NATIVE_PATH ${path}/${QT_VERSION}/QtGui)
            set(QT_NATIVE_PATH ${QT_NATIVE_PATH} PARENT_SCOPE)
            include_directories(AFTER ${QT_NATIVE_PATH})
        endif()
    endforeach(path)

    find_package(Threads REQUIRED)
    find_package(XCB REQUIRED)
ENDIF(UNIX)

# List of all source files. It may be possible to have the build process call cmake to update the makefiles
# only when this file has changed (ie source files have been added or removed)

set( SOURCES
    "constants.h"
    "main.cpp"
    "version.cpp"
    "version.h"
    "io/viewer_io.h"
    "io/viewer_io.cpp"
    "io/camera_controllers.h"
    "io/camera_controllers.cpp"
    "io/cad_camera_controller.h"
    "io/cad_camera_controller.cpp"
    "io/fps_camera_controller.h"
    "io/fps_camera_controller.cpp"
    "io/axis_free_camera_controller.h"
    "io/axis_free_camera_controller.cpp"
    "managers/load_animation_manager.cpp"
    "managers/load_animation_manager.h"
    "managers/message_manager.cpp"
    "managers/message_manager.h"
    "managers/navigation_manager.cpp"
    "managers/navigation_manager.h"
    "managers/pane_manager.cpp"
    "managers/pane_manager.h"
    "managers/trace_manager.cpp"
    "managers/trace_manager.h"
    "models/acceleration_structure_tree_view_item_delegate.cpp"
    "models/acceleration_structure_tree_view_item_delegate.h"
    "models/acceleration_structure_tree_view_item.cpp"
    "models/acceleration_structure_tree_view_item.h"
    "models/acceleration_structure_tree_view_model.cpp"
    "models/acceleration_structure_tree_view_model.h"
    "models/acceleration_structure_viewer_model.cpp"
    "models/acceleration_structure_viewer_model.h"
    "models/scene.cpp"
    "models/scene.h"
    "models/scene_node.cpp"
    "models/scene_node.h"
    "models/scene_collection_model.cpp"
    "models/scene_collection_model.h"
    "models/table_item_delegate.cpp"
    "models/table_item_delegate.h"
    "models/table_proxy_model.cpp"
    "models/table_proxy_model.h"
    "models/tree_view_proxy_model.cpp"
    "models/tree_view_proxy_model.h"
    "models/viewer_container_model.cpp"
    "models/viewer_container_model.h"
    "models/blas/blas_scene_collection_model.cpp"
    "models/blas/blas_scene_collection_model.h"
    "models/blas/blas_instances_model.cpp"
    "models/blas/blas_instances_model.h"
    "models/blas/blas_properties_model.cpp"
    "models/blas/blas_properties_model.h"
    "models/blas/blas_triangles_item_model.cpp"
    "models/blas/blas_triangles_item_model.h"
    "models/blas/blas_triangles_model.cpp"
    "models/blas/blas_triangles_model.h"
    "models/blas/blas_triangles_proxy_model.cpp"
    "models/blas/blas_triangles_proxy_model.h"
    "models/blas/blas_triangles_table_item_delegate.cpp"
    "models/blas/blas_triangles_table_item_delegate.h"
    "models/blas/blas_geometries_item_model.cpp"
    "models/blas/blas_geometries_item_model.h"
    "models/blas/blas_geometries_model.cpp"
    "models/blas/blas_geometries_model.h"
    "models/blas/blas_geometries_proxy_model.cpp"
    "models/blas/blas_geometries_proxy_model.h"
    "models/blas/blas_geometries_table_item_delegate.cpp"
    "models/blas/blas_geometries_table_item_delegate.h"
    "models/blas/blas_viewer_model.cpp"
    "models/blas/blas_viewer_model.h"
    "models/overview/device_configuration_model.cpp"
    "models/overview/device_configuration_model.h"
    "models/overview/summary_model.cpp"
    "models/overview/summary_model.h"
    "models/side_panels/side_panel_model.h"
    "models/side_panels/view_model.cpp"
    "models/side_panels/view_model.h"
    "models/tlas/tlas_instances_model.cpp"
    "models/tlas/tlas_instances_model.h"
    "models/tlas/blas_list_item_model.cpp"
    "models/tlas/blas_list_item_model.h"
    "models/tlas/blas_list_model.cpp"
    "models/tlas/blas_list_model.h"
    "models/tlas/blas_list_proxy_model.cpp"
    "models/tlas/blas_list_proxy_model.h"
    "models/tlas/blas_list_table_item_delegate.cpp"
    "models/tlas/blas_list_table_item_delegate.h"
    "models/tlas/tlas_properties_model.cpp"
    "models/tlas/tlas_properties_model.h"
    "models/tlas/tlas_scene_collection_model.cpp"
    "models/tlas/tlas_scene_collection_model.h"
    "models/tlas/tlas_viewer_model.cpp"
    "models/tlas/tlas_viewer_model.h"
    "models/acceleration_structure_flags_table_item_delegate.cpp"
    "models/acceleration_structure_flags_table_item_delegate.h"
    "models/acceleration_structure_flags_table_item_model.cpp"
    "models/acceleration_structure_flags_table_item_model.h"
    "models/instance_list_table_item_delegate.cpp"
    "models/instance_list_table_item_delegate.h"
    "models/instances_item_model.cpp"
    "models/instances_item_model.h"
    "models/instances_proxy_model.cpp"
    "models/instances_proxy_model.h"
    "models/ray/ray_history_model.cpp"
    "models/ray/ray_history_model.h"
    "models/ray/ray_inspector_model.cpp"
    "models/ray/ray_inspector_model.h"
    "models/ray/ray_list_proxy_model.cpp"
    "models/ray/ray_list_proxy_model.h"
    "models/ray/ray_list_item_model.cpp"
    "models/ray/ray_list_item_model.h"
    "models/ray/ray_list_table_item_delegate.cpp"
    "models/ray/ray_list_table_item_delegate.h"
    "models/ray/ray_history_image_generator.cpp"
    "models/ray/ray_history_image_generator.h"
    "models/ray/ray_inspector_ray_list_item_delegate.cpp"
    "models/ray/ray_inspector_ray_list_item_delegate.h" 
    "models/ray/ray_inspector_ray_list_item_model.cpp"
    "models/ray/ray_inspector_ray_list_item_model.h"
    "models/ray/ray_inspector_ray_list_proxy_model.cpp"
    "models/ray/ray_inspector_ray_list_proxy_model.h"
    "models/ray/ray_inspector_scene_collection_model.cpp"
    "models/ray/ray_inspector_scene_collection_model.h"
    "models/ray/ray_counters_item_model.cpp"
    "models/ray/ray_counters_item_model.h"
    "settings/geometry_settings.cpp"
    "settings/geometry_settings.h"
    "settings/settings.cpp"
    "settings/settings.h"
    "settings/settings_reader.cpp"
    "settings/settings_reader.h"
    "settings/settings_writer.cpp"
    "settings/settings_writer.h"
    "util/file_util.cpp"
    "util/file_util.h"
    "util/log_file_writer.cpp"
    "util/log_file_writer.h"
    "views/acceleration_structure_tree_view.cpp"
    "views/acceleration_structure_tree_view.h"
    "util/rra_util.cpp"
    "util/rra_util.h"
    "util/string_util.cpp"
    "util/string_util.h"
    "views/acceleration_structure_viewer_pane.cpp"
    "views/acceleration_structure_viewer_pane.h"
    "views/base_pane.cpp"
    "views/base_pane.h"
    "views/debug_window.cpp"
    "views/debug_window.h"
    "views/debug_window.ui"
    "views/license_dialog.cpp"
    "views/license_dialog.h"
    "views/license_dialog.ui"
    "views/main_window.cpp"
    "views/main_window.h"
    "views/main_window.ui"
    "views/widget_util.cpp"
    "views/widget_util.h"
    "views/blas/blas_instances_pane.cpp"
    "views/blas/blas_instances_pane.h"
    "views/blas/blas_instances_pane.ui"
    "views/blas/blas_properties_pane.cpp"
    "views/blas/blas_properties_pane.h"
    "views/blas/blas_properties_pane.ui"
    "views/blas/blas_triangles_pane.cpp"
    "views/blas/blas_triangles_pane.h"
    "views/blas/blas_triangles_pane.ui"
    "views/blas/blas_geometries_pane.cpp"
    "views/blas/blas_geometries_pane.h"
    "views/blas/blas_geometries_pane.ui"
    "views/blas/blas_viewer_pane.cpp"
    "views/blas/blas_viewer_pane.h"
    "views/blas/blas_viewer_pane.ui"
    "views/custom_widgets/colored_checkbox.cpp"
    "views/custom_widgets/colored_checkbox.h"
    "views/custom_widgets/binary_checkbox.cpp"
    "views/custom_widgets/binary_checkbox.h"
    "views/custom_widgets/color_picker_button.cpp"
    "views/custom_widgets/color_picker_button.h"
    "views/custom_widgets/color_picker_widget.cpp"
    "views/custom_widgets/color_picker_widget.h"
    "views/custom_widgets/depth_slider_widget.cpp"
    "views/custom_widgets/depth_slider_widget.h"
    "views/custom_widgets/dispatch_legend.cpp"
    "views/custom_widgets/dispatch_legend.h"
    "views/custom_widgets/themes_and_colors_item_button.cpp"
    "views/custom_widgets/themes_and_colors_item_button.h"
    "views/custom_widgets/colored_radio_button.cpp"
    "views/custom_widgets/colored_radio_button.h"
    "views/custom_widgets/double_slider_heatmap_widget.cpp"
    "views/custom_widgets/double_slider_heatmap_widget.h"
    "views/custom_widgets/read_only_checkbox.cpp"
    "views/custom_widgets/read_only_checkbox.h"
    "views/custom_widgets/vertical_button_widget.cpp"
    "views/custom_widgets/vertical_button_widget.h"
    "views/custom_widgets/slider_style.h"
    "views/custom_widgets/rgp_histogram_widget.cpp"
    "views/custom_widgets/rgp_histogram_widget.h"
    "views/custom_widgets/ray_history_graphics_view.cpp"
    "views/custom_widgets/ray_history_graphics_view.h"
    "views/custom_widgets/dispatch_loading_bar.cpp"
    "views/custom_widgets/dispatch_loading_bar.h"
    "views/custom_widgets/icon_button.cpp"
    "views/custom_widgets/icon_button.h"
    "views/overview/device_configuration_pane.cpp"
    "views/overview/device_configuration_pane.h"
    "views/overview/device_configuration_pane.ui"
    "views/overview/dispatch_pane.cpp"
    "views/overview/dispatch_pane.h"
    "views/overview/dispatch_pane.ui"
    "views/overview/summary_pane.cpp"
    "views/overview/summary_pane.h"
    "views/overview/summary_pane.ui"
    "views/overview/tlas_pane.ui"
    "views/settings/settings_pane.cpp"
    "views/settings/settings_pane.h"
    "views/settings/settings_pane.ui"
    "views/settings/keyboard_shortcuts_pane.cpp"
    "views/settings/keyboard_shortcuts_pane.h"
    "views/settings/keyboard_shortcuts_pane.ui"
    "views/settings/themes_and_colors_pane.cpp"
    "views/settings/themes_and_colors_pane.h"
    "views/settings/themes_and_colors_pane.ui"
    "views/side_panels/side_pane_container.cpp"
    "views/side_panels/side_pane_container.h"
    "views/side_panels/side_pane_container.ui"
    "views/side_panels/view_pane.cpp"
    "views/side_panels/view_pane.h"
    "views/side_panels/view_pane.ui"
    "views/start/welcome_pane.cpp"
    "views/start/welcome_pane.h"
    "views/start/welcome_pane.ui"
    "views/start/recent_traces_pane.cpp"
    "views/start/recent_traces_pane.h"
    "views/start/recent_traces_pane.ui"
    "views/start/about_pane.cpp"
    "views/start/about_pane.h"
    "views/start/about_pane.ui"
    "views/tlas/tlas_instances_pane.cpp"
    "views/tlas/tlas_instances_pane.h"
    "views/tlas/tlas_instances_pane.ui"
    "views/tlas/blas_list_pane.cpp"
    "views/tlas/blas_list_pane.h"
    "views/tlas/blas_list_pane.ui"
    "views/tlas/tlas_properties_pane.cpp"
    "views/tlas/tlas_properties_pane.h"
    "views/tlas/tlas_properties_pane.ui"
    "views/tlas/tlas_viewer_pane.cpp"
    "views/tlas/tlas_viewer_pane.h"
    "views/tlas/tlas_viewer_pane.ui"
    "views/ray/ray_inspector_pane.cpp"
    "views/ray/ray_inspector_pane.h"
    "views/ray/ray_inspector_pane.ui"
    "views/ray/ray_history_pane.cpp"
    "views/ray/ray_history_pane.h"
    "views/ray/ray_history_pane.ui"
    "views/ray/ray_history_viewer_widget.ui"
    "views/viewer_container_widget.cpp"
    "views/viewer_container_widget.h"
    "views/viewer_container_widget.ui"
    "resources.qrc"
    "stylesheet.qss"
    ${UPDATECHECKAPI_SRC}
    ${UPDATECHECKAPI_INC}
    ${UPDATECHECKAPI_QT_SRC}
    ${UPDATECHECKAPI_QT_INC}
    ${UPDATECHECKAPI_QT_UI}
)

set( WINDOWS_SOURCES
    "windows/resource.h"
    "windows/rra.rc"
    "windows/rra_icon.ico"
)

# Filter out the UI files and get the list of generated files
set(UI_FILES ${SOURCES})
list(FILTER UI_FILES INCLUDE REGEX "\.ui$")
qt_wrap_ui (GENERATED_UI ${UI_FILES})
set(SOURCES ${SOURCES} ${GENERATED_UI})

# searching for library file

# Specify output executable name. For windows, indicate this is a Windows application
# and not a console application
IF(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${WINDOWS_SOURCES})
ELSEIF(APPLE)
    IF(NO_APP_BUNDLE)
        add_executable(${PROJECT_NAME} ${SOURCES})
    ELSE()
        set(MACOSX_BUNDLE_ICON_FILE rra.icns)
        set(MACOSX_ICON mac_osx/rra.icns)
        set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${MACOSX_ICON} ${SOURCES})
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_RPATH TRUE MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/mac_osx/MacOSXBundleInfo.plist.in)
    ENDIF()
ELSEIF(UNIX)
    add_executable(${PROJECT_NAME} ${SOURCES})
    # Setting RPATH here results in local build directory also being appended to RPATH
    # RPATH set instead by calling chrpath command line command for cleaner solution.
    # Correct cmake solution will likely require use of a separate make install
    # for packaging
    # Leaving these Commands here for future reference
    #    set_target_properties(${PROJECT_NAME} PROPERTIES
    #      BUILD_WITH_INSTALL_RPATH TRUE
    #      INSTALL_RPATH_USE_LINK_PATH TRUE
    #      INSTALL_RPATH "\$ORIGIN/qt/lib"
    #)
ENDIF(WIN32)

# Apply common developer tools target options and definitions
devtools_target_options(${PROJECT_NAME})

IF (WIN32 OR APPLE)
SOURCE_GROUP_BY_FOLDER(${PROJECT_NAME})
ENDIF()

# CMAKE_<CONFIG>_POSTFIX isn't applied automatically to executable targets so apply manually
IF(CMAKE_DEBUG_POSTFIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
ENDIF(CMAKE_DEBUG_POSTFIX)
IF(CMAKE_RELEASE_POSTFIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES RELEASE_POSTFIX ${CMAKE_RELEASE_POSTFIX})
ENDIF(CMAKE_RELEASE_POSTFIX)

# executable file library dependency list
IF(WIN32)
    target_link_libraries(${PROJECT_NAME} Backend Renderer rdf Qt::Widgets QtCustomWidgets QtUtils wsock32 winmm ${UPDATECHECKAPI_LIBS})
ELSEIF(UNIX)
    target_link_libraries(${PROJECT_NAME} Backend Renderer rdf Qt::Widgets QtCustomWidgets QtUtils Threads::Threads ${XCB_LIBRARIES} ${UPDATECHECKAPI_LIBS})
ENDIF()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "copying rtda to output directory"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RTDA_PATH} $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

IF(APPLE)
    IF (NOT NO_APP_BUNDLE)
        # Run macdeployqt if it can be found
        find_program(MACDEPLOYQT_EXECUTABLE NAMES macdeployqt)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # macdeployqt expects to be passed the "${PROJECT_NAME}.app" directory rather than the final target
            COMMAND ${MACDEPLOYQT_EXECUTABLE} $<TARGET_FILE_DIR:${PROJECT_NAME}>/../../
        )
    ENDIF()
ENDIF(APPLE)

IF(WIN32)
    # Copy the VisualStudio redist files
    # the list of all redist files is contained in the variable CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS. We only want to copy
    # a subset of these so we extract the directory from the first entry and then manually copy the files we want
    foreach(VSLIB ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        get_filename_component(VSREDISTDIR ${VSLIB} DIRECTORY)
        message("Visual Studio redistributable files directory = ${VSREDISTDIR}")
        break()
    endforeach()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "copying Visual Studio redist files from ${VSREDISTDIR} to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VSREDISTDIR}/msvcp140.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VSREDISTDIR}/msvcp140_1.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VSREDISTDIR}/concrt140.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VSREDISTDIR}/vcruntime140.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    IF (EXISTS ${VSREDISTDIR}/vcruntime140_1.dll)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VSREDISTDIR}/vcruntime140_1.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    ENDIF()

ENDIF(WIN32)

# Copy QT components into appropriate location in build destination
if (WIN32)
    DEPLOY_QT_BUILD(${PROJECT_NAME} TRUE ${PROJECT_BINARY_DIR})
elseif (UNIX AND NOT APPLE)
    DEPLOY_QT_BUILD(${PROJECT_NAME} TRUE ${PROJECT_BINARY_DIR})
elseif (APPLE)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        DEPLOY_QT_BUILD(${PROJECT_NAME} TRUE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        DEPLOY_QT_BUILD(${PROJECT_NAME} TRUE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
    endif()
endif()

IF(UNIX AND NOT APPLE)
    IF(NOT QT_SYSTEM)
        # Call chrpath on system to override executable file RPATH.
        find_program(CHRPATH_EXECUTABLE NAMES chrpath)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "trying chrpath"
            COMMAND ${CHRPATH_EXECUTABLE} -r "\\$$ORIGIN/lib" $<TARGET_FILE:${PROJECT_NAME}>
        )
    ENDIF(NOT QT_SYSTEM)

ENDIF(UNIX AND NOT APPLE)

