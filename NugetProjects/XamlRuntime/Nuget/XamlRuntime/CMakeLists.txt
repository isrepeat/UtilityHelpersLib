add_library(utility_helpers_xaml_runtime STATIC
    XamlRuntime.Shared/XamlRuntime/Binding.cpp
    XamlRuntime.Shared/XamlRuntime/Binding.h
    XamlRuntime.Shared/XamlRuntime/DependentProperty.h
    XamlRuntime.Shared/XamlRuntime/Animation.cpp
    XamlRuntime.Shared/XamlRuntime/VisualState.cpp
    XamlRuntime.Shared/XamlRuntime/VisualState.h
    XamlRuntime.Shared/XamlRuntime/Animation.h
    XamlRuntime.Shared/XamlRuntime/ElementBuilder.cpp
    XamlRuntime.Shared/XamlRuntime/ElementBuilder.h
    XamlRuntime.Shared/XamlRuntime/Input.cpp
    XamlRuntime.Shared/XamlRuntime/Input.h
    XamlRuntime.Shared/XamlRuntime/GestureRecognizer.cpp
    XamlRuntime.Shared/XamlRuntime/GestureRecognizer.h
    XamlRuntime.Shared/XamlRuntime/InteractionController.cpp
    XamlRuntime.Shared/XamlRuntime/InteractionController.h
    XamlRuntime.Shared/XamlRuntime/RenderEngine.cpp
    XamlRuntime.Shared/XamlRuntime/RenderEngine.h
    XamlRuntime.Shared/XamlRuntime/ScrollController.cpp
    XamlRuntime.Shared/XamlRuntime/ScrollController.h
    XamlRuntime.Shared/XamlRuntime/XamlLayout.cpp
    XamlRuntime.Shared/XamlRuntime/XamlLayout.h
    XamlRuntime.Shared/XamlRuntime/UserControl.cpp
    XamlRuntime.Shared/XamlRuntime/UserControl.h
)
add_library(utility_helpers::xaml_runtime ALIAS utility_helpers_xaml_runtime)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/XamlRuntime.Shared" FILES
    XamlRuntime.Shared/XamlRuntime/Binding.cpp
    XamlRuntime.Shared/XamlRuntime/Binding.h
    XamlRuntime.Shared/XamlRuntime/DependentProperty.h
    XamlRuntime.Shared/XamlRuntime/Animation.cpp
    XamlRuntime.Shared/XamlRuntime/VisualState.cpp
    XamlRuntime.Shared/XamlRuntime/VisualState.h
    XamlRuntime.Shared/XamlRuntime/Animation.h
    XamlRuntime.Shared/XamlRuntime/ElementBuilder.cpp
    XamlRuntime.Shared/XamlRuntime/ElementBuilder.h
    XamlRuntime.Shared/XamlRuntime/Input.cpp
    XamlRuntime.Shared/XamlRuntime/Input.h
    XamlRuntime.Shared/XamlRuntime/GestureRecognizer.cpp
    XamlRuntime.Shared/XamlRuntime/GestureRecognizer.h
    XamlRuntime.Shared/XamlRuntime/InteractionController.cpp
    XamlRuntime.Shared/XamlRuntime/InteractionController.h
    XamlRuntime.Shared/XamlRuntime/RenderEngine.cpp
    XamlRuntime.Shared/XamlRuntime/RenderEngine.h
    XamlRuntime.Shared/XamlRuntime/ScrollController.cpp
    XamlRuntime.Shared/XamlRuntime/ScrollController.h
    XamlRuntime.Shared/XamlRuntime/XamlLayout.cpp
    XamlRuntime.Shared/XamlRuntime/XamlLayout.h
    XamlRuntime.Shared/XamlRuntime/UserControl.cpp
    XamlRuntime.Shared/XamlRuntime/UserControl.h
)
target_include_directories(utility_helpers_xaml_runtime PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/XamlRuntime.Shared>"
)
target_link_libraries(utility_helpers_xaml_runtime PUBLIC utility_helpers::logging)
set_target_properties(utility_helpers_xaml_runtime PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)