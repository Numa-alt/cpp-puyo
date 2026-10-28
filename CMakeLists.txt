cmake_minimum_required(VERSION 3.20)

project(cpp_puyo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)

add_executable(cpp_puyo
    main.cpp
    font8x8.cpp
)

target_link_libraries(cpp_puyo PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)