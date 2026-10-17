cmake_minimum_required(VERSION 3.16)

project(Rex VERSION 1.0
        DESCRIPTION "Lightweight app launcher"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB   REQUIRED IMPORTED_TARGET xcb xcb-keysyms)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo cairo-xcb)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango pangocairo)

add_subdirectory(src)
