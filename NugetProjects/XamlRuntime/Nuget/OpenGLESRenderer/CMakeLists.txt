add_library(utility_helpers_open_gles_renderer STATIC
    OpenGLESRenderer.Shared/ESRenderer/OpenGlRenderer.cpp
    OpenGLESRenderer.Shared/ESRenderer/OpenGlRenderer.h
    ThirdParty/stb_truetype.h
)
add_library(utility_helpers::open_gles_renderer ALIAS utility_helpers_open_gles_renderer)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/OpenGLESRenderer.Shared" FILES
    OpenGLESRenderer.Shared/ESRenderer/OpenGlRenderer.cpp
    OpenGLESRenderer.Shared/ESRenderer/OpenGlRenderer.h
)
source_group("ThirdParty" FILES ThirdParty/stb_truetype.h)

target_include_directories(utility_helpers_open_gles_renderer PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/OpenGLESRenderer.Shared>"
)
target_link_libraries(utility_helpers_open_gles_renderer PUBLIC
    utility_helpers::xaml_runtime
)
if (ANDROID)
    target_link_libraries(utility_helpers_open_gles_renderer PUBLIC GLESv3)
endif()
set_target_properties(utility_helpers_open_gles_renderer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)