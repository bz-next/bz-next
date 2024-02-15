cmake_minimum_required(VERSION 3.5)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(MAPVIEWER_BZFLAG_TEXTURE_RESOURCES "${PROJECT_SOURCE_DIR}/data/texresources.conf")
corrade_add_resource(BUNDLEDMAPS_RESOURCES resources/resources.conf)

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-mconsole")
endif()

add_executable(mapviewer WIN32 mapviewer.cpp ${MAPVIEWER_BZFLAG_TEXTURE_RESOURCES} ${BUNDLEDMAPS_RESOURCES}
    defaultBZDB.cxx
    defaultBZDB.h)
target_link_libraries(mapviewer PRIVATE
    Corrade::Main
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Trade
    Magnum::Primitives
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::DebugTools
    MagnumIntegration::ImGui
    Magnum::Sdl2Application
    ${ZLIB_LIBRARIES}
    Threads::Threads
    bzcommon
    bz3D
    bznet
    bzplatform
    bzobstacle
    bzscene
    bzmediafile
    bzdate
    bzwreader
    bzgame
    bzgfx
    bzwidgets)

if (CORRADE_TARGET_EMSCRIPTEN)
    target_link_libraries(mapviewer PRIVATE Magnum::AnyImageImporter Magnum::AnyImageConverter MagnumPlugins::PngImporter Magnum::AnySceneImporter)
endif()

target_compile_options(mapviewer PRIVATE -w)
add_dependencies(mapviewer Magnum::AnyImageImporter Magnum::AnyImageConverter MagnumPlugins::PngImporter Magnum::AnySceneImporter)

install(TARGETS mapviewer DESTINATION ${MAGNUM_BINARY_INSTALL_DIR})

if(CORRADE_TARGET_EMSCRIPTEN)
    install(TARGETS mapviewer DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES
    mapviewer.html
        ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
        ${MAGNUM_WEBAPPLICATION_CSS}
        DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(FILES
        #${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mapviewer.js.mem
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mapviewer.wasm
        DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()