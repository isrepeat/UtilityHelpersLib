file(GLOB_RECURSE utility_helpers_new_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared/HelpersNew/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared/HelpersNew/*.h"
)

add_library(utility_helpers_new STATIC ${utility_helpers_new_source_files})
add_library(utility_helpers::new ALIAS utility_helpers_new)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared" FILES
    ${utility_helpers_new_source_files}
)

target_include_directories(utility_helpers_new PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared>"
)
set_target_properties(utility_helpers_new PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)