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

set(RA95_DESKTOP_LAN_SOURCES
    CODE/NETDLG.CPP
    PORT/MAC/src/lan_udp.cpp
)

if(APPLE)
    add_executable(redalert_mac
        PORT/MAC/src/main.cpp
        ${RA95_PLATFORM_SDL_SOURCES}
        ${RA95_SOURCES}
        ${RA95_SUPPORT_SOURCES}
        ${RA95_DESKTOP_LAN_SOURCES}
    )

    ra95_configure_target(redalert_mac)
    target_compile_definitions(redalert_mac PRIVATE RA95_NATIVE_SOCKETS WINSOCK_IPX)

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
        ${RA95_DESKTOP_LAN_SOURCES}
    )

    ra95_configure_target(redalert_linux)

    target_compile_definitions(redalert_linux PRIVATE
        RA_LINUX
        RA95_NATIVE_SOCKETS
        WINSOCK_IPX
    )

    target_link_libraries(redalert_linux PRIVATE
        PkgConfig::SDL2
        Threads::Threads
    )
else()
    message(FATAL_ERROR "The desktop CMake target currently supports macOS and Linux only.")
endif()

add_executable(lan_udp_test
    tests/lan_udp_test.cpp
    PORT/MAC/src/lan_udp.cpp
)
ra95_configure_target(lan_udp_test)
target_compile_definitions(lan_udp_test PRIVATE RA95_NATIVE_SOCKETS WINSOCK_IPX)

add_executable(lan_packet_test tests/lan_packet_test.cpp)
ra95_configure_target(lan_packet_test)
target_compile_options(lan_packet_test PRIVATE "-iquote${RA95_CASEFOLD_INCLUDE_ROOT}/code")
target_compile_definitions(lan_packet_test PRIVATE RA95_NATIVE_SOCKETS WINSOCK_IPX)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # GCC emits unused legacy file-class vtables through function.h; the
    # packet-only test must not require the rest of the engine to link them.
    target_compile_options(lan_packet_test PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(lan_packet_test PRIVATE -Wl,--gc-sections)
endif()
enable_testing()
add_test(NAME lan_udp COMMAND lan_udp_test)
add_test(NAME lan_packet COMMAND lan_packet_test)
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
