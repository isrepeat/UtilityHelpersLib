# Android application template

1. Copy every file below this directory to the root of a new repository.
2. Replace `<Application>`, `<application>` and `<APPLICATION>`.
3. Rename `_Application_.Android` to the chosen Android module name and add the
   native CMake targets.
4. From `Tools/Gradle`, run `gradle wrapper --gradle-version 9.5` once to create
   the Gradle wrapper binary and scripts, then commit the generated wrapper files.
5. Run `./build.ps1 build-android -Configuration Debug`.

The common build implementation remains in AndroidBuildTools. Do not copy files from
`tools` into the new repository.