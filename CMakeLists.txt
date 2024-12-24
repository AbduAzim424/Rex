cmake_minimum_required(VERSION 3.30)

project(Rex VERSION 1.0
        DESCRIPTION "Lightweight app launcher"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -Wextra -pedantic -Wreorder")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

add_subdirectory(src)

find_package(PkgConfig REQUIRED)

pkg_check_modules(XCB REQUIRED xcb xcb-keysyms)
pkg_check_modules(CAIRO REQUIRED cairo cairo-xcb)
pkg_check_modules(PANGO REQUIRED pango pangocairo)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${XCB_INCLUDE_DIRS} 
    ${CAIRO_INCLUDE_DIRS}
    ${PANGO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${XCB_LIBRARIES} 
    ${CAIRO_LIBRARIES}
    ${PANGO_LIBRARIES}
    xcb
    xcb-keysyms
)