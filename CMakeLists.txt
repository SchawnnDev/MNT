cmake_minimum_required(VERSION 3.10)
project(MNT C)

find_package(MPI REQUIRED)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
set(CMAKE_C_STANDARD 99)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(MNT src/main.c src/check.h src/darboux.c src/darboux.h src/io.h src/io.c src/type.h)

target_link_libraries(MNT ${MPI_C_LIBRARIES})