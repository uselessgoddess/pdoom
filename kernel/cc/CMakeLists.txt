cmake_minimum_required(VERSION 3.20)
project(doom C)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "-fPIC -ffreestanding -nostdlib --target=x86_64-unknown-none")

file(GLOB SOURCES DOOM/*.c)
add_library(doom STATIC ${SOURCES}
        main.c)
target_link_libraries(doom PUBLIC DOOM)

install(TARGETS doom)
