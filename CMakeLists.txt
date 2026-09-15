cmake_minimum_required(VERSION 3.22.1)

# При add_subdirectory() имя и языки проекта задаёт потребитель (MobileClock).
# project() нужен только при самостоятельной CMake-сборке UtilityHelpersLib.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(utility_helpers LANGUAGES CXX)
endif()

# Каждый сторонний компонент сам описывает CMake-таргеты рядом с исходниками.
add_subdirectory(3rdParty/Spdlog/Spdlog.Shared)
add_subdirectory(Helpers/Helpers.Logging)
add_subdirectory(NugetProjects/XamlRuntime/Nuget/XamlRuntime)
add_subdirectory(HelpersNew)
add_subdirectory(NugetProjects/XamlRuntime/Nuget/OpenGLESRenderer)