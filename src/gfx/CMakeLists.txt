add_library(gfx STATIC
    MagnumTextureManager.cpp
)

add_dependencies(gfx Magnum::AnyImageImporter MagnumPlugins::PngImporter)

target_link_libraries(gfx PRIVATE
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
    MagnumIntegration::ImGui)