cmake_minimum_required(VERSION 3.20)
project(render CXX)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "-msoft-float -mno-sse -m64 -fPIC -ffreestanding -nostdlib --target=x86_64-unknown-illumos")

add_library(render STATIC ${SOURCES}
        main.cxx)

target_include_directories(render PUBLIC "std")
# target_include_directories(render PUBLIC "std/bits")

install(TARGETS render)
