# Post-build CPack script to reshape component archives to include a top-level directory

set(_folder_name "${CPACK_ARCHIVE_APPS_FILE_NAME}")
set(_is_zip FALSE)

# Use CPACK_PACKAGE_FILES to find the apps archive for this packaging pass.
# With CPACK_ARCHIVE_COMPONENT_INSTALL ON and CPACK_COMPONENTS_GROUPING IGNORE,
# this script is called once per component, so we must skip passes that don't
# include the apps archive to avoid reprocessing an already-reshaped archive.
# Match against the known archive suffixes rather than using NAME_WE/NAME_WLE:
# the file name contains version dots (e.g. "RRA-1.2.3"), which the extension
# parsers would strip incorrectly.
set(_apps_archive "")
foreach(_pkg_file IN LISTS CPACK_PACKAGE_FILES)
    get_filename_component(_pkg_name "${_pkg_file}" NAME)
    foreach(_suffix ".zip" ".tar.gz" ".tgz")
        if (_pkg_name STREQUAL "${_folder_name}${_suffix}")
            set(_apps_archive "${_pkg_file}")
            break()
        endif()
    endforeach()
    if (NOT _apps_archive STREQUAL "")
        break()
    endif()
endforeach()

# Fallback: CPACK_PACKAGE_FILES is not populated by all CPack generators/versions
# for post-build scripts. When it is unavailable, search the packaging output
# directory directly (the prior behavior).
if (_apps_archive STREQUAL "" AND (NOT DEFINED CPACK_PACKAGE_FILES OR "${CPACK_PACKAGE_FILES}" STREQUAL ""))
    foreach(_suffix ".zip" ".tar.gz" ".tgz")
        if (EXISTS "${CPACK_TEMPORARY_DIRECTORY}/../${_folder_name}${_suffix}")
            set(_apps_archive "${CPACK_TEMPORARY_DIRECTORY}/../${_folder_name}${_suffix}")
            break()
        endif()
    endforeach()
endif()

if (_apps_archive STREQUAL "")
    message(STATUS "Post-packaging: apps archive not in this pass, skipping.")
    return()
endif()

if (_apps_archive MATCHES "\\.zip$")
    set(_is_zip TRUE)
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
    file(REMOVE_RECURSE "${_repackage_dir}")
    return()
endif()

# Guard against reprocessing: if the extracted contents already contain a
# top-level directory matching the folder name, the archive was reshaped on a
# prior pass. Bail out without modifying it to avoid nesting <name>/<name>/...
if (EXISTS "${_repackage_dir}/${_folder_name}")
    message(STATUS "Post-packaging: '${_apps_archive}' already reshaped, skipping.")
    file(REMOVE_RECURSE "${_repackage_dir}")
    return()
endif()

# Create the reshaped archive to a temporary path first, so the original is only
# replaced after a successful re-pack. A failed re-pack must not leave the build
# without an apps archive.
set(_tmp_archive "${_apps_archive}.reshaped")
file(REMOVE "${_tmp_archive}")
message(STATUS "Creating new ${CPACK_GENERATOR} '${_tmp_archive}'")
if (_is_zip)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${_tmp_archive}" "${_folder_name}" --format=zip
        WORKING_DIRECTORY "${_out_dir}"
        RESULT_VARIABLE _c_res)
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${_tmp_archive}" "${_folder_name}"
        WORKING_DIRECTORY "${_out_dir}"
        RESULT_VARIABLE _c_res)
endif()
if (NOT _c_res EQUAL 0)
    message(WARNING "Failed to create reshaped archive ${_tmp_archive}: code ${_c_res}")
    file(REMOVE "${_tmp_archive}")
    file(REMOVE_RECURSE "${_repackage_dir}")
    return()
endif()

# Re-pack succeeded; atomically replace the original archive.
file(RENAME "${_tmp_archive}" "${_apps_archive}")
file(REMOVE_RECURSE "${_repackage_dir}")


