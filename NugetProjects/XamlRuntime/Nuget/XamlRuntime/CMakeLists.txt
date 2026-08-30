# Platform-specific static library delivered by the XamlRuntime package.
add_library(utility_helpers_xaml_runtime STATIC
    XamlRuntime.Shared/XamlRuntime/XamlLayout.cpp
    XamlRuntime.Shared/XamlRuntime/XamlLayout.h
)
add_library(utility_helpers::xaml_runtime ALIAS utility_helpers_xaml_runtime)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/XamlRuntime.Shared" FILES
    XamlRuntime.Shared/XamlRuntime/XamlLayout.cpp
    XamlRuntime.Shared/XamlRuntime/XamlLayout.h
)

target_include_directories(utility_helpers_xaml_runtime PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/XamlRuntime.Shared>"
)
set_target_properties(utility_helpers_xaml_runtime PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)