cmake_minimum_required(VERSION 3.17)
project(babyFEM_cpp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

include_directories(${CMAKE_SOURCE_DIR}/src)

# Executable principal
add_executable(fem_solver
    src/main.cpp
)

# Tests/exemples
add_executable(example_traction examples/traction.cpp)
add_executable(example_shear examples/shear.cpp)
add_executable(example_test_glace examples/test_glace.cpp)
