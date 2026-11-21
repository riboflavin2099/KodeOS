set(PORT "${CMAKE_CURRENT_LIST_DIR}/../port")
set(CHOC "${CMAKE_CURRENT_LIST_DIR}/../third_party/chocolate-doom/src")

idf_component_register(
    SRCS
        "app_main.c"
        "app_ao.c"
        "doom_wad.c"
        "doom_layout.c"
        "doom_file.c"
        "doom_save.c"
        "doom_input.c"
        "doom_audio.c"
        "doom_video.c"
        "doom_font.c"
        "doom_game_select.c"
        "doom_menu.c"
        "doom_engine.c"
        "doom_frame.c"
    INCLUDE_DIRS "." "${PORT}" "${CHOC}"
    REQUIRES kode kdrv esp_timer
)

# Early expansion only records requirements. The game archives are real targets.
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
file(GLOB CHOC_COMMON "${CHOC}/*.c")
file(GLOB CHOC_DOOM "${CHOC}/doom/*.c")
file(GLOB CHOC_HERETIC "${CHOC}/heretic/*.c")
file(GLOB CHOC_HEXEN "${CHOC}/hexen/*.c")
list(FILTER CHOC_COMMON EXCLUDE REGEX "/(net_gui|z_native|d_dedicated)\\.c$")
list(FILTER CHOC_DOOM EXCLUDE REGEX "_icon\\.c$")
list(FILTER CHOC_HERETIC EXCLUDE REGEX "_icon\\.c$")
list(FILTER CHOC_HEXEN EXCLUDE REGEX "_icon\\.c$")

# Chocolate Doom is vendored at the commit in third_party/chocolate-doom/UPSTREAM.md.
# Heretic and Hexen share names with Doom, so their archives are prefixed after
# compile. The zone and the view live in PSRAM inside the port sources above.
set(DOOM_GAME_WARN
    -Wno-error
    -Wno-unused-parameter
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -Wno-sign-compare
    -Wno-pointer-sign
    -Wno-misleading-indentation
    -Wno-override-init
    -Wno-unused-const-variable
    -Wno-type-limits
    -Wno-absolute-value
    -Wno-implicit-fallthrough
    -Wno-old-style-declaration
    -Wno-missing-field-initializers
    -Wno-enum-conversion
    -Wno-shift-negative-value
    -Wno-format
    -std=gnu17
    -fno-lto
)

function(doom_add_game target prefix)
    add_library(${target} STATIC ${ARGN})
    target_include_directories(${target} BEFORE PRIVATE ${PORT} ${prefix} ${CHOC})
    target_compile_options(${target} PRIVATE ${DOOM_GAME_WARN})
    target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
endfunction()

doom_add_game(chocodoom "${CHOC}/doom" ${CHOC_COMMON} ${CHOC_DOOM})
doom_add_game(heretic "${CHOC}/heretic" ${CHOC_HERETIC})
doom_add_game(hexen "${CHOC}/hexen" ${CHOC_HEXEN})

if(NOT CMAKE_NM)
    set(CMAKE_NM nm)
endif()
if(NOT CMAKE_OBJCOPY)
    set(CMAKE_OBJCOPY objcopy)
endif()
if(NOT CMAKE_AR)
    set(CMAKE_AR ar)
endif()

set(PORT_SRC
    "${PORT}/doom_port.c"
    "${PORT}/i_cdmus.c"
    "${PORT}/i_endoom.c"
    "${PORT}/i_input.c"
    "${PORT}/i_joystick.c"
    "${PORT}/i_sound.c"
    "${PORT}/i_system.c"
    "${PORT}/i_timer.c"
    "${PORT}/i_video.c"
    "${PORT}/i_videohr.c"
    "${PORT}/net_stub.c"
    "${PORT}/sdl_stub.c"
    "${PORT}/txt_stub.c"
)
add_library(doomport STATIC ${PORT_SRC})
target_include_directories(doomport BEFORE PUBLIC "${PORT}" "${CHOC}" "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(doomport PUBLIC idf::freertos idf::esp_timer idf::heap idf::log)
target_compile_options(doomport PRIVATE -Wall -Wextra -Werror -Wno-error=unused-function
    -Wno-error=unused-variable -Wno-unused-parameter -std=gnu17)
target_compile_definitions(doomport PRIVATE _GNU_SOURCE)

if(NOT CMAKE_OBJDUMP)
    set(CMAKE_OBJDUMP objdump)
endif()

function(doom_finish_archive target symbol_prefix)
    if("${symbol_prefix}" STREQUAL "")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND python3 "${PORT}/prefix_objs.py"
                --archive "$<TARGET_FILE:${target}>"
                --move-bss
                --nm "${CMAKE_NM}"
                --objcopy "${CMAKE_OBJCOPY}"
                --objdump "${CMAKE_OBJDUMP}"
                --ar "${CMAKE_AR}"
            VERBATIM
        )
    else()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND python3 "${PORT}/prefix_objs.py"
                --archive "$<TARGET_FILE:${target}>"
                --prefix "${symbol_prefix}"
                --move-bss
                --nm "${CMAKE_NM}"
                --objcopy "${CMAKE_OBJCOPY}"
                --objdump "${CMAKE_OBJDUMP}"
                --ar "${CMAKE_AR}"
            VERBATIM
        )
    endif()
endfunction()

doom_finish_archive(chocodoom "")
doom_finish_archive(heretic "heretic_")
doom_finish_archive(hexen "hexen_")

# CMake does not enable WHOLE_ARCHIVE until the language variables exist.
foreach(lang C CXX)
    set(CMAKE_${lang}_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)
    set(CMAKE_${lang}_LINK_LIBRARY_USING_WHOLE_ARCHIVE
        "LINKER:--push-state,--whole-archive"
        "<LINK_ITEM>"
        "LINKER:--pop-state")
endforeach()

# Whole archives so each game can see the shared code and the port.
target_link_libraries(${COMPONENT_LIB} PUBLIC
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,chocodoom>"
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,heretic>"
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,hexen>"
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,doomport>"
)
add_dependencies(${COMPONENT_LIB} chocodoom heretic hexen doomport)
endif()
