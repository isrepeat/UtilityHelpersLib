set({{application}}_project_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
set({{application}}_generated_root "${{{application}}_project_root}/!Generated/{{Application}}.Application/Xaml")
execute_process(
    COMMAND powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass
        -File "${{{application}}_project_root}/build.ps1" -Command generate-xaml
    WORKING_DIRECTORY "${{{application}}_project_root}"
    COMMAND_ERROR_IS_FATAL ANY
)
file(GLOB_RECURSE {{application}}_application_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Base/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Base/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Interface/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Model/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Model/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/UI/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/UI/*.h")
file(GLOB_RECURSE {{application}}_generated_xaml CONFIGURE_DEPENDS "${{{application}}_generated_root}/*.xaml.cpp")
if (NOT {{application}}_generated_xaml)
    message(FATAL_ERROR "XamlCompiler did not generate application sources.")
endif()
add_library({{application}}_application STATIC ${{{application}}_application_sources} ${{{application}}_generated_xaml})
set_target_properties({{application}}_application PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features({{application}}_application PUBLIC cxx_std_20)
target_include_directories({{application}}_application PUBLIC "${{{application}}_project_root}")
target_link_libraries({{application}}_application PUBLIC XamlRuntime::Runtime)
if (NOT ANDROID)
    target_compile_definitions({{application}}_application PUBLIC ANDROID_APP_PREVIEWER)
endif()
if (MSVC)
    target_compile_options({{application}}_application PRIVATE /utf-8)
endif()