set(SHADER_SOURCES
    BasicTexturedShader.cpp
    DepthMapShader.cpp
    DepthMapVisualizerShader.cpp
)

target_sources(bzgfx PRIVATE ${SHADER_SOURCES} ${SHADER_RESOURCES})
target_include_directories(bzgfx PUBLIC include/)
