# ANGLE.Windows.x64

Release build of ANGLE for native Windows x64 projects. The package provides EGL and OpenGL ES headers, import libraries, and the required runtime DLLs. Its MSBuild target adds the include and library paths, links `libEGL.lib` and `libGLESv2.lib`, and copies `libEGL.dll`, `libGLESv2.dll`, and `z.dll` next to the executable.