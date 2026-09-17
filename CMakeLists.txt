cmake_minimum_required(VERSION 3.22.1)

# При add_subdirectory() имя и языки проекта задаёт потребитель (MobileClock).
# project() нужен только при самостоятельной CMake-сборке UtilityHelpersLib.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(utility_helpers LANGUAGES CXX)
endif()

# XamlRuntime всегда приходит готовым NuGet-пакетом: его config создаёт
# imported CMake targets для Runtime, Renderer, Logging и ANGLE.
if (NOT EXISTS "${XAML_RUNTIME_PACKAGE_ROOT}/build/native/cmake/XamlRuntimeConfig.cmake")
    message(FATAL_ERROR "Set XAML_RUNTIME_PACKAGE_ROOT to the extracted XamlRuntime NuGet package.")
endif()
include("${XAML_RUNTIME_PACKAGE_ROOT}/build/native/cmake/XamlRuntimeConfig.cmake")
add_subdirectory(HelpersNew)