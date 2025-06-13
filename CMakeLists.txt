cmake_minimum_required(VERSION 3.31)
project(flamealloc C)

set(CMAKE_C_STANDARD 23)

add_library(flamealloc STATIC library.c)
