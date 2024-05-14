cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
project(Analyze)

# 设置默认构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

AUX_SOURCE_DIRECTORY(./src src)
add_executable(main ${src})

# 设置 DEBUG 模式下的编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "DEBUG mode: setting optimization level to -O0")
    target_compile_options(main PRIVATE -O0)
else()
    # Release 模式下的编译选项
    message(STATUS "Release mode: setting optimization level to -O2")
    target_compile_options(main PRIVATE -O2)
endif()
