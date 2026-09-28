set({{application}}_project_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
set({{application}}_generated_root "${{{application}}_project_root}/!Generated/{{Application}}.Application/Xaml")
set({{application}}_xaml_generator "${{{application}}_project_root}/build.ps1")

# Generated XAML — обычный C++-исходник и должен существовать до сканирования CMake.
execute_process(
    COMMAND powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass
        -File "${{{application}}_xaml_generator}" -Command generate-xaml
    WORKING_DIRECTORY "${{{application}}_project_root}"
    COMMAND_ERROR_IS_FATAL ANY
)
file(GLOB_RECURSE {{application}}_generated_xaml CONFIGURE_DEPENDS
    "${{{application}}_generated_root}/*.xaml.cpp")
if (NOT {{application}}_generated_xaml)
    message(FATAL_ERROR "XamlCompiler did not generate MainPage sources.")
endif()

add_library({{application}} SHARED Native.cpp ${{{application}}_generated_xaml})
target_compile_features({{application}} PRIVATE cxx_std_20)
target_include_directories({{application}} PRIVATE "${{{application}}_project_root}")
target_link_libraries({{application}} PRIVATE XamlRuntime::Runtime)
if (ANDROID)
    set_target_properties({{application}} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/{{Application}}.AndroidHost/android/jniLibs/${ANDROID_ABI}")
endif()