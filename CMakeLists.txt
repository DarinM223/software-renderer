cmake_minimum_required(VERSION 3.28)
project(software-renderer)

include_directories(include/)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,leak,undefined")
set(CMAKE_C_STANDARD 11)
# Use this to avoid having to #undef NDEBUG in every file.
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(day1)
target_sources(day1 PUBLIC src/day1.c)
