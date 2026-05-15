# Post-build CPack script to reshape component archives to include a top-level directory

set(_folder_name "${CPACK_ARCHIVE_APPS_FILE_NAME}")
set(_is_tgz FALSE)
set(_is_zip FALSE)

# Use CPACK_PACKAGE_FILES to find the apps archive for this packaging pass.
# With CPACK_ARCHIVE_COMPONENT_INSTALL ON and CPACK_COMPONENTS_GROUPING IGNORE,
# this script is called once per component, so we must skip passes that don't
# include the apps archive to avoid reprocessing an already-reshaped archive.
# NAME_WLE strips all extensions (e.g. "foo.tar.gz" -> "foo"), correctly
# handling the double-extension used by the TGZ generator on Linux.
set(_apps_archive "")
foreach(_pkg_file IN LISTS CPACK_PACKAGE_FILES)
    get_filename_component(_pkg_stem "${_pkg_file}" NAME_WLE)
    if (_pkg_stem STREQUAL _folder_name)
        set(_apps_archive "${_pkg_file}")
        break()
    endif()
endforeach()

if (_apps_archive STREQUAL "")
    message(STATUS "Post-packaging: apps archive not in this pass, skipping.")
    return()
endif()

if (_apps_archive MATCHES "\\.zip$")
    set(_is_zip TRUE)
else()
    set(_is_tgz TRUE)
endif()

message(STATUS "Post-packaging: Setting archive top-level directory to '${_folder_name}'")

# _out_dir = _CPack_Packages/Linux/TGZ etc.
set(_out_dir "${CPACK_TEMPORARY_DIRECTORY}/..")

# Create top-level directory
set(_repackage_dir "${_out_dir}/${_folder_name}")
message(STATUS "Creating ${_repackage_dir}")
file(REMOVE_RECURSE "${_repackage_dir}")
file(MAKE_DIRECTORY "${_repackage_dir}")

# Extract using cmake -E tar (gz) or zip
message(STATUS "Extracting ${CPACK_GENERATOR} to '${_repackage_dir}'")
if (_is_zip)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${_apps_archive}" --format=zip
        WORKING_DIRECTORY "${_repackage_dir}"
        RESULT_VARIABLE _x_res)
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xzf "${_apps_archive}"
        WORKING_DIRECTORY "${_repackage_dir}"
        RESULT_VARIABLE _x_res)
endif()
if (NOT _x_res EQUAL 0)
    message(WARNING "Failed to extract ${_apps_archive}: code ${_x_res}")
    return()
endif()

# Remove the original archive
message(STATUS "Removing original '${_apps_archive}'")
file(REMOVE "${_apps_archive}")

# Create new archive
message(STATUS "Creating new ${CPACK_GENERATOR} '${_apps_archive}'")
if (_is_zip)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${_apps_archive}" "${_folder_name}" --format=zip
        WORKING_DIRECTORY "${_out_dir}"
        RESULT_VARIABLE _c_res)
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${_apps_archive}" "${_folder_name}"
        WORKING_DIRECTORY "${_out_dir}"
        RESULT_VARIABLE _c_res)
endif()
if (NOT _c_res EQUAL 0)
    message(WARNING "Failed to create reshaped archive ${_apps_archive}: code ${_c_res}")
    return()
endif()

