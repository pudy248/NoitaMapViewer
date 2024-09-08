cmake_minimum_required(VERSION 3.28)
project(NoitaMapViewer)

set(CMAKE_CXX_STANDARD 26)

find_package(SFML REQUIRED system window graphics)
find_package(PNG REQUIRED)

include_directories(.)

add_executable(NoitaMapViewer
        binops.hpp
        colors.hpp
        fastlz.c
        fastlz.h
        png_petri_main.cpp
        pngutils.h
        utils.h)

target_link_libraries(NoitaMapViewer
        sfml-graphics sfml-system sfml-window
        PNG::PNG
        )

