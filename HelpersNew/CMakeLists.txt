add_library(utility_helpers_new STATIC
    HelpersNew.Shared/HelpersNew/Filesystem/ReadAllBytes.cpp
    HelpersNew.Shared/HelpersNew/Filesystem/ReadAllBytes.h
    HelpersNew.Shared/HelpersNew/Geometry/ContainsPoint.cpp
    HelpersNew.Shared/HelpersNew/Geometry/ContainsPoint.h
)
add_library(utility_helpers::new ALIAS utility_helpers_new)
target_include_directories(utility_helpers_new PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared>"
)
set_target_properties(utility_helpers_new PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)