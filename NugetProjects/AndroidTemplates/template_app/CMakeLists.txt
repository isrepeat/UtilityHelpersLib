cmake_minimum_required(VERSION 3.22.1)
project({{Application}} LANGUAGES C CXX)

# Desktop presets не проходят через AndroidToolchain.cmake, поэтому до
# подключения CMake-модулей восстанавливаем тот же закреплённый пакет tools.
if (NOT ANDROID_BUILD_TOOLS_ROOT)
    find_program({{application}}_bootstrap_powershell NAMES powershell.exe REQUIRED)
    execute_process(
        COMMAND "${{{application}}_bootstrap_powershell}" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/build.ps1" -Command restore
        RESULT_VARIABLE {{application}}_bootstrap_result
        OUTPUT_VARIABLE {{application}}_bootstrap_output
        ERROR_VARIABLE {{application}}_bootstrap_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT {{application}}_bootstrap_result EQUAL 0)
        message(FATAL_ERROR "AndroidBuildTools restore failed: ${{{application}}_bootstrap_error}")
    endif()
    string(REGEX MATCH "[^\r\n]+$" {{application}}_bootstrap_root
        "${{{application}}_bootstrap_output}")
    file(TO_CMAKE_PATH "${{{application}}_bootstrap_root}" ANDROID_BUILD_TOOLS_ROOT)
endif()

function({{application}}_get_configuration_path configuration_name output_variable)
    find_program({{application}}_powershell NAMES powershell.exe REQUIRED)
    execute_process(
        COMMAND "${{{application}}_powershell}" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/build.ps1"
            -Command get-configuration-path -Name "${configuration_name}"
        RESULT_VARIABLE {{application}}_configuration_result
        OUTPUT_VARIABLE {{application}}_configuration_output
        ERROR_VARIABLE {{application}}_configuration_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT {{application}}_configuration_result EQUAL 0)
        message(FATAL_ERROR "Could not read ${configuration_name}: ${{{application}}_configuration_error}")
    endif()
    string(REGEX MATCH "[^\r\n]+$" {{application}}_configuration_path
        "${{{application}}_configuration_output}")
    file(TO_CMAKE_PATH "${{{application}}_configuration_path}" {{application}}_configuration_path)
    set(${output_variable} "${{{application}}_configuration_path}" PARENT_SCOPE)
endfunction()

function({{application}}_get_configuration_value configuration_name output_variable)
    find_program({{application}}_powershell NAMES powershell.exe REQUIRED)
    execute_process(
        COMMAND "${{{application}}_powershell}" -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_CURRENT_SOURCE_DIR}/build.ps1"
            -Command get-configuration-value -Name "${configuration_name}"
        RESULT_VARIABLE {{application}}_configuration_result
        OUTPUT_VARIABLE {{application}}_configuration_output
        ERROR_VARIABLE {{application}}_configuration_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT {{application}}_configuration_result EQUAL 0)
        message(FATAL_ERROR "Could not read ${configuration_name}: ${{{application}}_configuration_error}")
    endif()
    string(REGEX MATCH "[^\r\n]+$" {{application}}_configuration_value
        "${{{application}}_configuration_output}")
    set(${output_variable} "${{{application}}_configuration_value}" PARENT_SCOPE)
endfunction()

{{application}}_get_configuration_path("PackageDirectories.XamlRuntime"
    {{application}}_xaml_runtime_packages_root)
{{application}}_get_configuration_value("PackageSources.Native" {{application}}_native_package_source)
include("${ANDROID_BUILD_TOOLS_ROOT}/tools/cmake/InstallXamlRuntime.cmake")
fn_androidappkit_install_xaml_runtime("${{{application}}_xaml_runtime_packages_root}"
    "${{{application}}_native_package_source}")

add_subdirectory({{Application}}.AndroidHost)
if (NOT ANDROID)
    {{application}}_get_configuration_path("PackageDirectories.AndroidAppPreviewerPluginSdk"
        {{application}}_preview_plugin_sdk_packages_root)
    include("${ANDROID_BUILD_TOOLS_ROOT}/tools/cmake/InstallAndroidAppPreviewerPluginSdk.cmake")
    fn_androidappkit_install_android_app_previewer_plugin_sdk(
        "${{{application}}_preview_plugin_sdk_packages_root}"
        "${{{application}}_native_package_source}")
    add_subdirectory({{Application}}.PreviewPlugin)
endif()