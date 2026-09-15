# Исходники spdlog уже закреплены в репозитории. Это header-only target: CMake
# передаёт потребителю include-путь, но не создаёт вторую копию библиотеки.
add_library(utility_helpers_spdlog INTERFACE)
add_library(utility_helpers::spdlog ALIAS utility_helpers_spdlog)
target_include_directories(utility_helpers_spdlog INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Spdlog"
)