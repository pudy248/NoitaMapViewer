cmake_minimum_required(VERSION 3.28)
project(NoitaMapViewer)

set(CMAKE_CXX_STANDARD 23)

find_package(SFML REQUIRED system window graphics)
find_package(PNG REQUIRED)

include_directories(.)

add_executable(NoitaMapViewer
        binops.h
        chunks.h
        edit.h
        fastlz.c
        fastlz.h
        file_access.h
        materials.h
        png_petri.h
        pngutils.h
        streaminfo.h
        wak.h
        winapi_wrappers.h
        main.cpp)

target_link_libraries(NoitaMapViewer
        sfml-graphics sfml-system sfml-window
        PNG::PNG
        )
