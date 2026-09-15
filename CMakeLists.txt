cmake_minimum_required(VERSION 3.28)
project(software-renderer)

find_package(OpenMP REQUIRED)
include_directories(include/)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,leak,undefined")
set(CMAKE_C_STANDARD 11)
# Use this to avoid having to #undef NDEBUG in every file.
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic -g -fsanitize=address,leak,undefined")

add_executable(day1)
target_sources(day1 PUBLIC src/day1.c)

add_executable(day2)
target_sources(day2 PUBLIC src/day2.c)
target_compile_options(day2 PUBLIC -O3 -march=native)
target_link_libraries(day2 m)

add_executable(day2_wireframe)
target_sources(day2_wireframe PUBLIC src/day2_wireframe.c src/model.c)
target_compile_options(day2_wireframe PUBLIC -O3 -march=native)
target_link_libraries(day2_wireframe m)

add_executable(day3)
target_sources(day3 PUBLIC src/day3.c src/model.c)
target_link_libraries(day3 m OpenMP::OpenMP_C)