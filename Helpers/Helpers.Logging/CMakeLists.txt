add_library(utility_helpers_logging STATIC
    Helpers.Logging/Logging.cpp
    Helpers.Logging/Logging.h
    Helpers.Logging/LoggingBackend.h
)
if (ANDROID)
    target_sources(utility_helpers_logging PRIVATE Helpers.Logging/platform/Android/LoggingBackend.cpp)
    find_library(utility_helpers_android_log log)
    target_link_libraries(utility_helpers_logging PUBLIC ${utility_helpers_android_log})
else()
    target_sources(utility_helpers_logging PRIVATE Helpers.Logging/platform/Windows/LoggingBackend.cpp)
endif()
add_library(utility_helpers::logging ALIAS utility_helpers_logging)
target_include_directories(utility_helpers_logging PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
target_link_libraries(utility_helpers_logging PUBLIC utility_helpers::spdlog)
set_target_properties(utility_helpers_logging PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)