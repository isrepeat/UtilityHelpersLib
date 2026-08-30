# Исходники spdlog уже закреплены в репозитории. Это header-only target: CMake
# передаёт потребителю include-путь, но не создаёт вторую копию библиотеки.
add_library(utility_helpers_spdlog INTERFACE)
add_library(utility_helpers::spdlog ALIAS utility_helpers_spdlog)
target_include_directories(utility_helpers_spdlog INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Spdlog"
)

# Android-обёртка намеренно живёт рядом со spdlog: её единственная задача —
# настроить android_sink и дать приложению небольшой независимый API.
if (ANDROID)
    add_library(utility_helpers_android_logging STATIC
        platform/Android/AndroidLogging.cpp
    )
    add_library(utility_helpers::android_logging ALIAS utility_helpers_android_logging)

    # Путь нужен для #include <platform/Android/AndroidLogging.h> у потребителя.
    target_include_directories(utility_helpers_android_logging PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    )
    # compile_features не добавил -std к самому вложенному CMake-таргету в
    # текущем preset. Фиксируем стандарт явно, чтобы Clang и IntelliSense
    # одинаково видели string_view, once_flag и shared_ptr.
    set_target_properties(utility_helpers_android_logging PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
    )

    # android_sink вызывает функции из NDK liblog, а spdlog остаётся
    # header-only транзитивной зависимостью публичного заголовка.
    find_library(utility_helpers_android_log log)
    target_link_libraries(utility_helpers_android_logging PUBLIC
        utility_helpers::spdlog
        ${utility_helpers_android_log}
    )
endif()
