cmake_minimum_required(VERSION 3.10)
project(MNT C)

find_package(MPI REQUIRED)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3 -march=native -g -fopenmp")
set(CMAKE_C_STANDARD 99)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

add_executable(MNT src/main.c src/check.h src/darboux.c src/darboux.h src/io.h src/io.c src/type.h)

target_link_libraries(MNT ${MPI_C_LIBRARIES})

if(MPI_COMPILE_FLAGS)
    set_target_properties(MNT PROPERTIES
            COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(MNT PROPERTIES
            LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()