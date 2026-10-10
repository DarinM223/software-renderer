cmake_minimum_required(VERSION 3.28)
project(software-renderer)

find_package(OpenMP REQUIRED)
include_directories(include/)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,leak,undefined")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -g")
set(CMAKE_C_STANDARD 11)
# Use this to avoid having to #undef NDEBUG in every file.
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


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

add_executable(day4)
target_sources(day4 PUBLIC src/day4.c)
target_link_libraries(day4 m OpenMP::OpenMP_C)

add_executable(day5)
target_sources(day5 PUBLIC src/day5.c src/model.c)
target_link_libraries(day5 m OpenMP::OpenMP_C)

add_executable(day6)
target_sources(day6 PUBLIC src/day6.c src/model.c)
target_link_libraries(day6 m OpenMP::OpenMP_C)

add_executable(day7)
target_sources(day7 PUBLIC src/day7.c src/model.c)
target_link_libraries(day7 m OpenMP::OpenMP_C)

add_executable(day8)
target_sources(day8 PUBLIC src/day8.c src/our_gl.c src/model.c)
target_link_libraries(day8 m OpenMP::OpenMP_C)

add_executable(day9)
target_sources(day9 PUBLIC src/day9.c src/our_gl.c src/model.c)
target_link_libraries(day9 m OpenMP::OpenMP_C)

add_executable(day10)
target_sources(day10 PUBLIC src/day10.c src/our_gl.c src/model.c)
target_link_libraries(day10 m OpenMP::OpenMP_C)

add_executable(day11)
target_sources(day11 PUBLIC src/day11.c src/our_gl.c src/model.c)
target_compile_options(day11 PUBLIC -O3 -march=native)
target_link_libraries(day11 m OpenMP::OpenMP_C)

add_executable(day12)
target_sources(day12 PUBLIC src/day12.c src/our_gl.c src/model.c)
target_compile_options(day12 PUBLIC -O3 -march=native)
target_link_libraries(day12 m OpenMP::OpenMP_C)

add_executable(day12_ssao)
target_sources(day12_ssao PUBLIC src/day12_ssao.c src/our_gl.c src/model.c)
target_compile_options(day12_ssao PUBLIC -O3 -march=native)
target_link_libraries(day12_ssao m OpenMP::OpenMP_C)

add_executable(day13)
target_sources(day13 PUBLIC src/day13.c src/our_gl.c src/model.c)
target_compile_options(day13 PUBLIC -O3 -march=native)
target_link_libraries(day13 m OpenMP::OpenMP_C)