cmake_minimum_required(VERSION 3.22)

project(red_alert LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

include(cmake/ra95_common.cmake)

if(APPLE)
    add_executable(redalert_mac
        PORT/MAC/src/main.cpp
        ${RA95_PLATFORM_SDL_SOURCES}
        ${RA95_SOURCES}
        ${RA95_SUPPORT_SOURCES}
    )

    ra95_configure_target(redalert_mac)

    target_link_libraries(redalert_mac PRIVATE
        PkgConfig::SDL2
        Threads::Threads
    )
elseif(UNIX)
    add_executable(redalert_linux
        PORT/MAC/src/main.cpp
        ${RA95_PLATFORM_SDL_SOURCES}
        ${RA95_SOURCES}
        ${RA95_SUPPORT_SOURCES}
    )

    ra95_configure_target(redalert_linux)

    target_compile_definitions(redalert_linux PRIVATE
        RA_LINUX
    )

    target_link_libraries(redalert_linux PRIVATE
        PkgConfig::SDL2
        Threads::Threads
    )
else()
    message(FATAL_ERROR "The desktop CMake target currently supports macOS and Linux only.")
endif()

enable_testing()
add_executable(connection_test tests/connection_test.cpp CODE/CONNECT.CPP CODE/COMBUF.CPP)
ra95_configure_target(connection_test)
target_compile_options(connection_test PRIVATE "-iquote${RA95_CASEFOLD_INCLUDE_ROOT}/code")
if(UNIX AND NOT APPLE)
    target_compile_options(connection_test PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(connection_test PRIVATE "LINKER:--gc-sections")
endif()
add_test(NAME connection COMMAND connection_test)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(NAME combat_rng COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_engine_regression.py" "${CMAKE_BINARY_DIR}")
