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
