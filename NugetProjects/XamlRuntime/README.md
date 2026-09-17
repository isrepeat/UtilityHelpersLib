# XamlRuntime package sources

`XamlRuntime` builds the platform-specific static UI runtime.
`OpenGLESRenderer` renders its frame through ANGLE on Windows and GLES on Android.
`XamlCompiler` is a Windows host tool that compiles `.xaml` pages.

`Scripts\Nuget.XamlRuntime.Pack.Release.cmd` creates `XamlRuntime.<version>.nupkg` in `NugetFeed`. It builds Windows x64 through MSBuild, ANGLE through the package-local vcpkg manifest, and Android variants through the NDK selected by `ANDROID_NDK_HOME`, `ANDROID_NDK_ROOT`, or `MobileClock.Android/local.properties`.

The package contains MSBuild integration under `build/native/XamlRuntime.targets` and Android CMake integration under `build/native/cmake/XamlRuntimeConfig.cmake`.

For Windows the package embeds the required ANGLE headers, import libraries and runtime DLLs. The vcpkg installed tree is generated under `XamlRuntime\!NUGET_TMP\ANGLE` and is not part of the repository or resulting package. Consumers do not need a separate ANGLE package or vcpkg.