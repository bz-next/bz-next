set(GFX_SOURCES
    Drawables.cpp
    IndexedMeshData.cpp
    WorldPrimitiveGenerator.cpp
    WorldRenderer.cpp
    WorldSceneBuilder.cpp
)

add_library(bzgfx STATIC ${GFX_SOURCES})
target_include_directories(bzgfx PUBLIC include/)

target_link_libraries(bzgfx
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