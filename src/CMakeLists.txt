add_executable(${PROJECT_NAME}
    launcher.cpp
    rex.cpp
    ui.cpp
    inputhandler.cpp
    suggestion.cpp
    desktopentry.cpp
    executionengine.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::XCB
    PkgConfig::CAIRO
    PkgConfig::PANGO
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
