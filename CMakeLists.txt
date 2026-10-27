cmake_minimum_required(VERSION 3.20)

project(cpp_puyo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cpp_puyo
    main.cpp
)

if (MSVC)
    target_compile_options(cpp_puyo PRIVATE /utf-8)
endif()