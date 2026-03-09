cmake_minimum_required(VERSION 3.14)
project(Axiom CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(axiom src/main.cpp)

add_executable(axiom_simulator simulator/main.cpp)

# FIX Gateway executable
add_executable(fix_gateway_test src/fix_gateway.cpp)