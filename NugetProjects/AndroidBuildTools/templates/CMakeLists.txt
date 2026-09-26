cmake_minimum_required(VERSION 3.22.1)
project(<Application> LANGUAGES C CXX)

# Android preset восстанавливает пакет через cmake/AndroidToolchain.cmake.
# Desktop-конфигурация восстанавливает его здесь.
if (NOT ANDROID_BUILD_TOOLS_ROOT)
    find_program(android_build_tools_powershell NAMES powershell.exe REQUIRED)
    execute_process(
        COMMAND "${android_build_tools_powershell}" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/build.ps1" -Command restore
        RESULT_VARIABLE android_build_tools_result
        OUTPUT_VARIABLE android_build_tools_output
        ERROR_VARIABLE android_build_tools_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT android_build_tools_result EQUAL 0)
        message(FATAL_ERROR "AndroidBuildTools restore failed: ${android_build_tools_output}\n${android_build_tools_error}")
    endif()
    string(REGEX MATCH "[^\r\n]+$" ANDROID_BUILD_TOOLS_ROOT "${android_build_tools_output}")
    file(TO_CMAKE_PATH "${ANDROID_BUILD_TOOLS_ROOT}" ANDROID_BUILD_TOOLS_ROOT)
endif()

include("${ANDROID_BUILD_TOOLS_ROOT}/tools/cmake/InstallXamlRuntime.cmake")
fn_androidappkit_install_xaml_runtime("<Application>.AndroidHost")

# Добавьте собственные native targets. Для desktop previewer подключите SDK:
# include("${ANDROID_BUILD_TOOLS_ROOT}/tools/cmake/InstallAndroidAppPreviewerPluginSdk.cmake")
# fn_androidappkit_install_android_app_previewer_plugin_sdk("<Application>.PreviewPlugin")
# add_subdirectory(<Application>.AndroidHost)