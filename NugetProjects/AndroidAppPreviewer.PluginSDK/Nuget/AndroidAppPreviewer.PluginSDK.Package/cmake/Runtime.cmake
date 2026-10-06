function(android_app_previewer_add_runtime target_name)
    set(preview_runtime_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../build/native/AndroidAppPreviewer.PluginSDK")
    file(GLOB_RECURSE preview_runtime_sources CONFIGURE_DEPENDS
        "${preview_runtime_root}/*.cpp"
        "${preview_runtime_root}/*.h")
    target_sources(${target_name} PRIVATE ${preview_runtime_sources})
    source_group(TREE "${preview_runtime_root}" PREFIX "Preview SDK" FILES ${preview_runtime_sources})
endfunction()