cmake_minimum_required(VERSION 3.20)

project(cpp_puyo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2)

# 警告オプション（core / platform 共通）
if(MSVC)
    set(PUYO_WARN_FLAGS /W4 /utf-8)
else()
    set(PUYO_WARN_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow)
endif()

# ---------------------------------------------------------
# コア: SDL に依存しない（Arduino 版でも使い回す部分）
# ---------------------------------------------------------
add_library(puyo_core STATIC
    core/Board.cpp
    core/InputManager.cpp
    core/PlayingManager.cpp
    core/GameManager.cpp
)
target_include_directories(puyo_core PUBLIC core)
target_compile_options(puyo_core PRIVATE ${PUYO_WARN_FLAGS})

# 組込み向けの制約を再現（例外・RTTI なし）
if(NOT MSVC)
    target_compile_options(puyo_core PRIVATE -fno-exceptions -fno-rtti)
endif()

# ---------------------------------------------------------
# PC 版（SDL2）
# ---------------------------------------------------------
add_executable(cpp_puyo
    platform_pc/main.cpp
    platform_pc/Effect.cpp
    platform_pc/Renderer.cpp
    platform_pc/font8x8.cpp
)
target_compile_options(cpp_puyo PRIVATE ${PUYO_WARN_FLAGS})
target_link_libraries(cpp_puyo PRIVATE
    puyo_core
    SDL2::SDL2main
    SDL2::SDL2
)