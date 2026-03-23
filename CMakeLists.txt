cmake_minimum_required(VERSION 3.31)
project(BasicComponents LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE BOOL "" FORCE)

add_executable(main)
target_sources(main PRIVATE main.cpp)
target_compile_features(main PRIVATE cxx_std_20)


add_subdirectory(ThreadPool)