cmake_minimum_required(VERSION 3.14)
project(hereed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_STANDARD_REQUIRED ON)

add_executable(hereed main.cc)

install(TARGETS hereed DESTINATION bin/)
