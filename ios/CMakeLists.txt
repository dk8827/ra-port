cmake_minimum_required(VERSION 3.22)

project(redalert_ios LANGUAGES C CXX OBJC)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(FATAL_ERROR "Configure the iOS target with -DCMAKE_SYSTEM_NAME=iOS, or use scripts/build_ios_debug.sh.")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "" FORCE)
endif()

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

set(RA95_IOS_VERSION "0.1.0" CACHE STRING "App version shown to users (CFBundleShortVersionString); bump by hand for releases.")

# The build number (CFBundleVersion) must grow with every build iOS and sideloading tools see as
# an update. Use the number of commits on the current branch, so each new commit builds a higher
# number. Pass -DRA95_IOS_BUILD_NUMBER=N to override it.
set(RA95_GIT_COMMIT "unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-list --count HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
        OUTPUT_VARIABLE RA95_GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
        OUTPUT_VARIABLE RA95_GIT_HEAD
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
        OUTPUT_VARIABLE RA95_GIT_CHANGES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(RA95_GIT_HEAD)
        set(RA95_GIT_COMMIT "${RA95_GIT_HEAD}")
        if(RA95_GIT_CHANGES)
            string(APPEND RA95_GIT_COMMIT "-dirty")
        endif()
    endif()
endif()
if(NOT DEFINED RA95_IOS_BUILD_NUMBER)
    if(RA95_GIT_COMMIT_COUNT MATCHES "^[0-9]+$")
        set(RA95_IOS_BUILD_NUMBER "${RA95_GIT_COMMIT_COUNT}")
    else()
        set(RA95_IOS_BUILD_NUMBER "1")
    endif()
endif()
message(STATUS "Red Alert iOS version ${RA95_IOS_VERSION} (${RA95_IOS_BUILD_NUMBER}), commit ${RA95_GIT_COMMIT}")

set(RA95_IOS_BUNDLE_ID "com.raport.redalert" CACHE STRING "iOS bundle identifier; device builds need one your Apple team can register.")
set(RA95_IOS_BUNDLED_ASSETS_DIR "${CMAKE_CURRENT_LIST_DIR}/../assets/redalert" CACHE PATH "Local Red Alert assets to bundle into the iOS debug app.")

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/third_party/SDL/CMakeLists.txt")
    message(FATAL_ERROR "Missing SDL2 sources at ios/third_party/SDL. Run scripts/build_ios_debug.sh to fetch them.")
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/third_party/SDL" "${CMAKE_BINARY_DIR}/SDL2")

include("${CMAKE_CURRENT_LIST_DIR}/../cmake/ra95_common.cmake")

add_executable(redalert_ios MACOSX_BUNDLE
    "${RA95_REPO_ROOT}/PORT/IOS/src/ios_app_delegate.m"
    "${RA95_REPO_ROOT}/PORT/IOS/src/ios_main.cpp"
    ${RA95_PLATFORM_SDL_SOURCES}
    ${RA95_SOURCES}
    ${RA95_SUPPORT_SOURCES}
)

ra95_configure_target(redalert_ios)

target_compile_definitions(redalert_ios PRIVATE
    RA_IOS
    RA_MOBILE_TOUCH
)

target_include_directories(redalert_ios PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/third_party/SDL/src/video/uikit"
)

if(TARGET SDL2::SDL2main)
    set(RA95_SDLMAIN_TARGET SDL2::SDL2main)
elseif(TARGET SDL2main)
    set(RA95_SDLMAIN_TARGET SDL2main)
else()
    message(FATAL_ERROR "SDL2main CMake target was not created")
endif()

if(TARGET SDL2::SDL2-static)
    set(RA95_SDL_TARGET SDL2::SDL2-static)
elseif(TARGET SDL2-static)
    set(RA95_SDL_TARGET SDL2-static)
elseif(TARGET SDL2::SDL2)
    set(RA95_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2)
    set(RA95_SDL_TARGET SDL2)
else()
    message(FATAL_ERROR "SDL2 CMake target was not created")
endif()

target_link_libraries(redalert_ios PRIVATE
    ${RA95_SDLMAIN_TARGET}
    ${RA95_SDL_TARGET}
)

set_target_properties(redalert_ios PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${RA95_IOS_BUNDLE_ID}"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
)

add_custom_command(TARGET redalert_ios POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:redalert_ios>/assets"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RA95_IOS_BUNDLED_ASSETS_DIR}" "$<TARGET_BUNDLE_DIR:redalert_ios>/assets/redalert"
    COMMENT "Copying local Red Alert assets into the iOS debug bundle"
)
