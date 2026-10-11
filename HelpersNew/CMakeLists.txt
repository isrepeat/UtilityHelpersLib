file(GLOB_RECURSE utility_helpers_new_source_files CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared/HelpersNew/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/HelpersNew.Shared/HelpersNew/*.h"
)

add_library(utility_helpers_new STATIC ${utility_helpers_new_source_files})
add_library(utility_helpers::new ALIAS utility_helpers_new)
target_compile_features(utility_helpers_new PUBLIC cxx_std_17)

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

option(HELPERS_NEW_BUILD_TESTS "Build HelpersNew tests" OFF)
if(HELPERS_NEW_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)
    add_executable(helpers_new_signal_tests Tests/SignalTests.cpp)
    target_link_libraries(helpers_new_signal_tests PRIVATE utility_helpers_new Threads::Threads)
    add_test(NAME helpers_new_signal_tests COMMAND helpers_new_signal_tests)
endif()