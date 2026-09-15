cmake_minimum_required(VERSION 3.22)
project(XamlCompiler LANGUAGES CXX)

add_executable(XamlCompiler
    src/main.cpp
)
set_target_properties(XamlCompiler PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)