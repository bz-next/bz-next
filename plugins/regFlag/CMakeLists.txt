# Get the current directory name
get_filename_component(name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Tell the build system what files we need to build for this plugin
add_library(${name} SHARED
    ${name}.cpp
)

# Remove the prefix from the name
set_target_properties(${name} PROPERTIES PREFIX "")

# Link against the exported symbols of bzfs
target_link_libraries(${name} bzfs)
