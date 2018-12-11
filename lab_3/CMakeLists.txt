cmake_minimum_required(VERSION 3.0)
SET(CMAKE_C_COMPILER "/usr/local/Cellar/gcc@6/6.4.0_1/bin/gcc-6")
SET(CMAKE_CXX_COMPILER "/usr/local/Cellar/gcc@6/6.4.0_1/bin/g++-6")
SET(CMAKE_CXX_STANDARD 11)
project(lab3)
set(SOURCE lab3.cpp)

add_executable(${PROJECT_NAME} ${SOURCE})