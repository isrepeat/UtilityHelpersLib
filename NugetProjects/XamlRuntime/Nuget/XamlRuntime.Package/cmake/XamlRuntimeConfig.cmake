include_guard(GLOBAL)

# Installed-package mode starts at build/native/cmake and walks up to the
# package root, where headers and native archives are laid out by Pack.ps1.
get_filename_component(_xaml_runtime_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if (WIN32)
    set(_xaml_runtime_configuration Release)
    if (CMAKE_BUILD_TYPE)
        set(_xaml_runtime_configuration "${CMAKE_BUILD_TYPE}")
    endif()
    set(_xaml_runtime_library_directory "${_xaml_runtime_prefix}/runtimes/win-x64/native/${_xaml_runtime_configuration}")
    add_library(XamlRuntime::Logging STATIC IMPORTED GLOBAL)
    set_target_properties(XamlRuntime::Logging PROPERTIES IMPORTED_LOCATION "${_xaml_runtime_library_directory}/Helpers.Logging.lib" INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include")
    add_library(XamlRuntime::Runtime STATIC IMPORTED GLOBAL)
    set_target_properties(XamlRuntime::Runtime PROPERTIES IMPORTED_LOCATION "${_xaml_runtime_library_directory}/XamlRuntime.lib" INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include" INTERFACE_LINK_LIBRARIES "XamlRuntime::Logging")
    add_library(XamlRuntime::OpenGLESRenderer STATIC IMPORTED GLOBAL)
    set_target_properties(XamlRuntime::OpenGLESRenderer PROPERTIES IMPORTED_LOCATION "${_xaml_runtime_library_directory}/OpenGLESRenderer.lib" INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include" INTERFACE_LINK_LIBRARIES "XamlRuntime::Runtime;${_xaml_runtime_library_directory}/libEGL.lib;${_xaml_runtime_library_directory}/libGLESv2.lib")
    add_library(utility_helpers::logging ALIAS XamlRuntime::Logging)
    add_library(utility_helpers::xaml_runtime ALIAS XamlRuntime::Runtime)
    add_library(utility_helpers::open_gles_renderer ALIAS XamlRuntime::OpenGLESRenderer)
    return()
endif()
if (NOT ANDROID)
    message(FATAL_ERROR "The CMake XamlRuntime package currently supports Android only. Use build/native/XamlRuntime.targets on Windows.")
endif()

# NuGet runtime identifiers describe the same architectures as Android ABIs,
# but use the conventional names expected by the package runtimes directory.
if (ANDROID_ABI STREQUAL "arm64-v8a")
    set(_xaml_runtime_rid android-arm64)
elseif (ANDROID_ABI STREQUAL "x86_64")
    set(_xaml_runtime_rid android-x64)
elseif (ANDROID_ABI STREQUAL "armeabi-v7a")
    set(_xaml_runtime_rid android-arm)
else()
    message(FATAL_ERROR "Unsupported Android ABI for XamlRuntime: ${ANDROID_ABI}")
endif()

# The package contains a separate directory of static libraries for each CMake
# configuration. Release remains the default when the consumer leaves
# CMAKE_BUILD_TYPE unset.
set(_xaml_runtime_configuration Release)
if (CMAKE_BUILD_TYPE)
    set(_xaml_runtime_configuration "${CMAKE_BUILD_TYPE}")
endif()
set(_xaml_runtime_library_directory "${_xaml_runtime_prefix}/runtimes/${_xaml_runtime_rid}/native/${_xaml_runtime_configuration}")

# Import all native components as named targets instead of exposing archive
# paths to the consumer. Their INTERFACE properties propagate headers and
# transitive link dependencies through target_link_libraries.
add_library(XamlRuntime::Logging STATIC IMPORTED GLOBAL)
set_target_properties(XamlRuntime::Logging PROPERTIES
    IMPORTED_LOCATION "${_xaml_runtime_library_directory}/libHelpers.Logging.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include"
    INTERFACE_LINK_LIBRARIES "log")

# Runtime markup uses the logging library; keeping the relation on the imported
# target makes XamlRuntime::Runtime sufficient for ordinary consumers.
add_library(XamlRuntime::Runtime STATIC IMPORTED GLOBAL)
set_target_properties(XamlRuntime::Runtime PROPERTIES
    IMPORTED_LOCATION "${_xaml_runtime_library_directory}/libXamlRuntime.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include"
    INTERFACE_LINK_LIBRARIES "XamlRuntime::Logging")

# Renderer builds on top of the runtime and links Android's GLESv3 system
# library. Consumers need only link XamlRuntime::OpenGLESRenderer.
add_library(XamlRuntime::OpenGLESRenderer STATIC IMPORTED GLOBAL)
set_target_properties(XamlRuntime::OpenGLESRenderer PROPERTIES
    IMPORTED_LOCATION "${_xaml_runtime_library_directory}/libOpenGLESRenderer.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_xaml_runtime_prefix}/build/native/include"
    INTERFACE_LINK_LIBRARIES "XamlRuntime::Runtime;GLESv3")
add_library(utility_helpers::logging ALIAS XamlRuntime::Logging)
add_library(utility_helpers::xaml_runtime ALIAS XamlRuntime::Runtime)
add_library(utility_helpers::open_gles_renderer ALIAS XamlRuntime::OpenGLESRenderer)