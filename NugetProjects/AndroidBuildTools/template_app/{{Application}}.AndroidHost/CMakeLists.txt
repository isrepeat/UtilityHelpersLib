add_library({{application}} SHARED Native.cpp)
target_compile_features({{application}} PRIVATE cxx_std_20)
if (ANDROID)
    set_target_properties({{application}} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/{{Application}}.AndroidHost/android/jniLibs/${ANDROID_ABI}")
endif()