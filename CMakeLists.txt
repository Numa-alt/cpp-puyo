cmake_minimum_required(VERSION 3.20)

project(cpp_puyo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)


add_executable(cpp_puyo
    main.cpp
    font8x8.cpp
)

if(MSVC)
    target_compile_options(cpp_puyo PRIVATE
        /W4
    )
else()
    target_compile_options(cpp_puyo PRIVATE
        -Wall
        -Wextra
        -Wconversion
        -Wsign-conversion
        -Wshadow
    )
endif()
target_link_libraries(cpp_puyo PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
)