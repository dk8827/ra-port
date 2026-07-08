cmake_minimum_required(VERSION 3.22)

project(redalert_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../third_party/SDL" "${CMAKE_BINARY_DIR}/SDL2")

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/ra95_common.cmake")

add_library(redalert_android SHARED
    "${RA95_REPO_ROOT}/PORT/ANDROID/src/android_main.cpp"
    ${RA95_PLATFORM_SDL_SOURCES}
    ${RA95_SOURCES}
    ${RA95_SUPPORT_SOURCES}
)

ra95_configure_target(redalert_android)

target_compile_definitions(redalert_android PRIVATE
    ANDROID
    RA_ANDROID
)

if(TARGET SDL2::SDL2)
    set(RA95_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2)
    set(RA95_SDL_TARGET SDL2)
else()
    message(FATAL_ERROR "SDL2 CMake target was not created")
endif()

target_link_libraries(redalert_android PRIVATE
    ${RA95_SDL_TARGET}
    android
    log
)

set_target_properties(redalert_android PROPERTIES
    OUTPUT_NAME main
)
