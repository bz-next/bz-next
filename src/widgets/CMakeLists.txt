set(WIDGETS_SOURCES
    BZMaterialBrowser.cpp
    BZMaterialViewer.cpp
    BZTextureBrowser.cpp
    ImGuiANSITextRenderer.cpp
    ObstacleBrowser.cpp
    BZChatConsole.cpp
    TextEditor.cpp
    BZWTextEditor.cpp
    AboutDialog.cpp
)

add_library(bzwidgets STATIC ${WIDGETS_SOURCES})
target_include_directories(bzwidgets PUBLIC include/)

target_link_libraries(bzwidgets
    Magnum::GL
    Magnum::Magnum
    Magnum::Shaders
    Magnum::Trade
    Magnum::Primitives
    Magnum::MeshTools
    Magnum::SceneGraph
    ${ZLIB_LIBRARIES}
    Threads::Threads
    MagnumIntegration::ImGui
)