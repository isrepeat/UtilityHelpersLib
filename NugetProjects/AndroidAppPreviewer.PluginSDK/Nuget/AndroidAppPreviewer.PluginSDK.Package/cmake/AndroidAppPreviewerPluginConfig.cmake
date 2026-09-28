include_guard(GLOBAL)

get_filename_component(AndroidAppPreviewerPlugin_PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if (NOT TARGET AndroidAppPreviewerPlugin::SDK)
    add_library(AndroidAppPreviewerPlugin::SDK INTERFACE IMPORTED)
    set_target_properties(AndroidAppPreviewerPlugin::SDK PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AndroidAppPreviewerPlugin_PACKAGE_PREFIX_DIR}/build/native/include")
endif()