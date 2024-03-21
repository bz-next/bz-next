set(SHADER_SOURCES
    BasicTexturedShader.cpp
    DepthMapShader.cpp
    DepthMapVisualizerShader.cpp
    EnhancedPhongGL.cpp
    RaymarchedCloudsShader.cpp
    DepthReinterpretShader.cpp
)

target_sources(bzgfx PRIVATE ${SHADER_SOURCES})
target_include_directories(bzgfx PUBLIC include/)
