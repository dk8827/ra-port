get_filename_component(RA95_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

include("${RA95_REPO_ROOT}/cmake/ra95_sources.cmake")

list(REMOVE_ITEM RA95_SOURCES
    CODE/KEYFRAME.CPP
    CODE/CCDDE.CPP
    CODE/DDE.CPP
    CODE/TCPIP.CPP
    CODE/LZWOTRAW.CPP
    CODE/MPLIB.CPP
    CODE/MPLPC.CPP
    CODE/MPMGRD.CPP
    CODE/MPMGRW.CPP
    CODE/NETDLG.CPP
    CODE/NULLCONN.CPP
    CODE/NULLMGR.CPP
    CODE/TARCOM.CPP
    CODE/TEMP.CPP
)

set(RA95_SUPPORT_SOURCES
    WIN32LIB/DRAWBUFF/BUFFER.CPP
    WIN32LIB/DRAWBUFF/BUFFGLBL.CPP
    WIN32LIB/DRAWBUFF/DRAWRECT.CPP
    WIN32LIB/DRAWBUFF/GBUFFER.CPP
    WIN32LIB/DRAWBUFF/ICONCACH.CPP
    WIN32LIB/DRAWBUFF/REGIONSZ.CPP
    WIN32LIB/FONT/FONT.CPP
    WIN32LIB/FONT/LOADFONT.CPP
    WIN32LIB/FONT/SET_FONT.CPP
    WIN32LIB/IFF/IFF.CPP
    WIN32LIB/IFF/LOAD.CPP
    WIN32LIB/KEYBOARD/MOUSE.CPP
    WIN32LIB/MEM/ALLOC.CPP
    WIN32LIB/MEM/MEM.CPP
    WIN32LIB/MISC/DDRAW.CPP
    WIN32LIB/MISC/DELAY.CPP
    WIN32LIB/MISC/EXIT.CPP
    WIN32LIB/MISC/FINDARGV.CPP
    WIN32LIB/MISC/IRANDOM.CPP
    WIN32LIB/MISC/LIB.CPP
    WIN32LIB/MISC/VERSION.CPP
    WIN32LIB/PLAYCD/GETCD.CPP
    WIN32LIB/SHAPE/GETSHAPE.CPP
    WIN32LIB/SHAPE/PRIOINIT.CPP
    WIN32LIB/TILE/ICONSET.CPP
    WIN32LIB/TIMER/TIMER.CPP
    WIN32LIB/TIMER/TIMERDWN.CPP
    WIN32LIB/WSA/WSA.CPP
)

list(TRANSFORM RA95_SOURCES PREPEND "${RA95_REPO_ROOT}/")
list(TRANSFORM RA95_SUPPORT_SOURCES PREPEND "${RA95_REPO_ROOT}/")

set(RA95_PLATFORM_SDL_SOURCES
    "${RA95_REPO_ROOT}/PORT/MAC/src/mac_audio_stub.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/mac_sdl_runtime.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/mac_timer.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/mac_vqa.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/dos_compat.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/ccdde_stub.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/tcpip_stub.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/legacy_primitives.cpp"
)

set(RA95_INCLUDE_DIRS
    "${RA95_REPO_ROOT}/PORT/MAC/include"
    "${RA95_REPO_ROOT}/WIN32LIB/INCLUDE"
    "${RA95_REPO_ROOT}/WIN32LIB/DRAWBUFF"
    "${RA95_REPO_ROOT}/WIN32LIB/AUDIO"
    "${RA95_REPO_ROOT}/WIN32LIB/FONT"
    "${RA95_REPO_ROOT}/WIN32LIB/IFF"
    "${RA95_REPO_ROOT}/WIN32LIB/MISC"
    "${RA95_REPO_ROOT}/WIN32LIB/KEYBOARD"
    "${RA95_REPO_ROOT}/WIN32LIB/MEM"
    "${RA95_REPO_ROOT}/WIN32LIB/MONO"
    "${RA95_REPO_ROOT}/WIN32LIB/PALETTE"
    "${RA95_REPO_ROOT}/WIN32LIB/PLAYCD"
    "${RA95_REPO_ROOT}/WIN32LIB/RAWFILE"
    "${RA95_REPO_ROOT}/WIN32LIB/SHAPE"
    "${RA95_REPO_ROOT}/WIN32LIB/TILE"
    "${RA95_REPO_ROOT}/WIN32LIB/TIMER"
    "${RA95_REPO_ROOT}/WIN32LIB/WSA"
    "${RA95_REPO_ROOT}/CODE"
    "${RA95_REPO_ROOT}/CODE/ENG"
    "${RA95_REPO_ROOT}/CODE/WOLAPI"
    "${RA95_REPO_ROOT}/WWFLAT32/INCLUDE"
    "${RA95_REPO_ROOT}/WWFLAT32/MCGAPRIM"
    "${RA95_REPO_ROOT}/WWFLAT32/AUDIO"
    "${RA95_REPO_ROOT}/WWFLAT32/FILE"
    "${RA95_REPO_ROOT}/WWFLAT32/FONT"
    "${RA95_REPO_ROOT}/WWFLAT32/IFF"
    "${RA95_REPO_ROOT}/WWFLAT32/MISC"
    "${RA95_REPO_ROOT}/WWFLAT32/PALETTE"
    "${RA95_REPO_ROOT}/WWFLAT32/SHAPE"
    "${RA95_REPO_ROOT}/WWFLAT32/TILE"
    "${RA95_REPO_ROOT}/WWFLAT32/TIMER"
    "${RA95_REPO_ROOT}/WWFLAT32/VIDEO"
    "${RA95_REPO_ROOT}/WWFLAT32/WINDOWS"
    "${RA95_REPO_ROOT}/WWFLAT32/WSA"
    "${RA95_REPO_ROOT}/WINVQ/INCLUDE"
    "${RA95_REPO_ROOT}/WINVQ/INCLUDE/WWLIB32"
    "${RA95_REPO_ROOT}/WINVQ/INCLUDE/VQM32"
    "${RA95_REPO_ROOT}/VQ/INCLUDE"
    "${RA95_REPO_ROOT}/VQ/INCLUDE/WWLIB32"
    "${RA95_REPO_ROOT}/VQ/INCLUDE/VQM32"
)

set(RA95_CASEFOLD_INCLUDE_ROOT "${CMAKE_BINARY_DIR}/ra95_casefold_include")
execute_process(
    COMMAND
        "${RA95_REPO_ROOT}/scripts/create_casefold_include_overlay.sh"
        "${RA95_CASEFOLD_INCLUDE_ROOT}"
        PORT/MAC/include
        CODE
        WIN32LIB
        WWFLAT32
        WINVQ/INCLUDE
        VQ/INCLUDE
    WORKING_DIRECTORY "${RA95_REPO_ROOT}"
    RESULT_VARIABLE RA95_CASEFOLD_INCLUDE_RESULT
    ERROR_VARIABLE RA95_CASEFOLD_INCLUDE_ERROR
)
if(NOT RA95_CASEFOLD_INCLUDE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to create casefold include overlay: ${RA95_CASEFOLD_INCLUDE_ERROR}")
endif()

set(RA95_CASEFOLD_INCLUDE_DIRS)
foreach(include_dir IN LISTS RA95_INCLUDE_DIRS)
    file(RELATIVE_PATH include_rel "${RA95_REPO_ROOT}" "${include_dir}")
    string(TOLOWER "${include_rel}" include_rel_lower)
    if(EXISTS "${RA95_CASEFOLD_INCLUDE_ROOT}/${include_rel_lower}")
        list(APPEND RA95_CASEFOLD_INCLUDE_DIRS "${RA95_CASEFOLD_INCLUDE_ROOT}/${include_rel_lower}")
    endif()
endforeach()

set(RA95_COMPILE_DEFINITIONS
    TRUE_FALSE_DEFINED
    WIN32
    _WINDOWS
    ENGLISH
    INTERNET_OFF
    GAME_VERSION=0x00030003
    VQADIRECT_SOUND=1
)

set(RA95_COMPILE_OPTIONS
    -Wno-unknown-pragmas
    -Wno-multichar
)

set(RA95_CLANG_COMPILE_OPTIONS
    -Wno-writable-strings
    -Wno-deprecated-register
    -Wno-nonportable-include-path
)

set(RA95_GNU_COMPILE_OPTIONS
    -fpermissive
    -fno-access-control
    -Wno-write-strings
    -include
    "${RA95_REPO_ROOT}/PORT/MAC/include/legacy_compiler_compat.h"
)

set(RA95_CODE_QUOTE_SOURCES
    "${RA95_REPO_ROOT}/PORT/MAC/src/legacy_primitives.cpp"
    "${RA95_REPO_ROOT}/PORT/MAC/src/legacy_ops.cpp"
)

function(ra95_add_casefold_quote_includes target_name)
    get_target_property(ra95_target_sources ${target_name} SOURCES)
    foreach(source IN LISTS ra95_target_sources)
        if(source MATCHES "^\\$<")
            continue()
        endif()
        if(IS_ABSOLUTE "${source}")
            set(source_abs "${source}")
        else()
            get_filename_component(source_abs "${source}" ABSOLUTE BASE_DIR "${RA95_REPO_ROOT}")
        endif()
        if(NOT EXISTS "${source_abs}")
            continue()
        endif()

        get_filename_component(source_dir "${source_abs}" DIRECTORY)
        file(RELATIVE_PATH source_dir_rel "${RA95_REPO_ROOT}" "${source_dir}")
        string(TOLOWER "${source_dir_rel}" source_dir_rel_lower)
        set(source_casefold_dir "${RA95_CASEFOLD_INCLUDE_ROOT}/${source_dir_rel_lower}")
        if(EXISTS "${source_casefold_dir}")
            set_property(SOURCE "${source}" APPEND PROPERTY COMPILE_OPTIONS "-iquote${source_casefold_dir}")
            if(NOT source STREQUAL source_abs)
                set_property(SOURCE "${source_abs}" APPEND PROPERTY COMPILE_OPTIONS "-iquote${source_casefold_dir}")
            endif()
        endif()
        if(source_abs IN_LIST RA95_CODE_QUOTE_SOURCES)
            set(code_casefold_dir "${RA95_CASEFOLD_INCLUDE_ROOT}/code")
            if(EXISTS "${code_casefold_dir}")
                set_property(SOURCE "${source}" APPEND PROPERTY COMPILE_OPTIONS "-iquote${code_casefold_dir}")
                if(NOT source STREQUAL source_abs)
                    set_property(SOURCE "${source_abs}" APPEND PROPERTY COMPILE_OPTIONS "-iquote${code_casefold_dir}")
                endif()
            endif()
        endif()
    endforeach()
endfunction()

function(ra95_configure_target target_name)
    target_include_directories(${target_name} PRIVATE ${RA95_CASEFOLD_INCLUDE_DIRS} ${RA95_INCLUDE_DIRS})
    target_compile_definitions(${target_name} PRIVATE ${RA95_COMPILE_DEFINITIONS})
    target_compile_options(${target_name} PRIVATE ${RA95_COMPILE_OPTIONS})
    ra95_add_casefold_quote_includes(${target_name})
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target_name} PRIVATE ${RA95_CLANG_COMPILE_OPTIONS})
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE ${RA95_GNU_COMPILE_OPTIONS})
    endif()
endfunction()
