include_directories(${CMAKE_CURRENT_SOURCE_DIR}/plugin_utils)

# Based on answer from https://stackoverflow.com/questions/53619372/dynamic-subdirectories-in-a-cmake-build
FILE(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
SET(plugindirs "")
FOREACH(subdir ${subdirs})
    # TODO: Skip SAMPLE_PLUGIN for non-debug builds
    IF(
        IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}
        AND
        EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt
    )
        SET(plugindirs ${plugindirs} ${subdir})
    ENDIF()
ENDFOREACH()

FOREACH(plugindir ${plugindirs})
    add_subdirectory(${plugindir})
ENDFOREACH()

