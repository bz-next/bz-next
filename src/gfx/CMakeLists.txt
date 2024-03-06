set(GFX_SOURCES
    Drawables.cpp
    IndexedMeshData.cpp
    WorldPrimitiveGenerator.cpp
    WorldSceneObjectGenerator.cpp
    WorldMeshGenerator.cpp
    GLInfo.cpp
    DrawableGroupManager.cpp
    SceneObjectManager.cpp
    DrawMode.cpp
    DrawModeManager.cpp
    MagnumSceneRenderer.cpp
)

add_library(bzgfx STATIC ${GFX_SOURCES})
target_include_directories(bzgfx PUBLIC include/)

add_subdirectory(SceneObjects)
add_subdirectory(Shaders)

target_link_libraries(bzgfx
    Magnum::Application
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Trade
    Magnum::Primitives
    Magnum::MeshTools
    Magnum::SceneGraph
    ${ZLIB_LIBRARIES}
    Threads::Threads
)