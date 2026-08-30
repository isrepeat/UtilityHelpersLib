if (ANDROID)
    add_library(utility_helpers_android_logging STATIC
        Helpers/platform/Android/Logging.cpp
        Helpers/platform/Android/Logging.h
    )
    add_library(utility_helpers::android_logging ALIAS utility_helpers_android_logging)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
        Helpers/platform/Android/Logging.cpp
        Helpers/platform/Android/Logging.h
    )

    target_include_directories(utility_helpers_android_logging PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    )
    set_target_properties(utility_helpers_android_logging PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    find_library(utility_helpers_android_log log)
    target_link_libraries(utility_helpers_android_logging PUBLIC
        utility_helpers::spdlog
        ${utility_helpers_android_log}
    )
endif()