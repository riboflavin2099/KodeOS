idf_component_register(
    SRCS
        "app_main.c"
        "app_ao.c"
        "gbc_header.c"
        "gbc_rom.c"
        "gbc_save.c"
        "gbc_input.c"
        "gbc_audio.c"
        "gbc_mix.c"
        "gbc_display.c"
        "gbc_core.c"
        "gbc_frame.c"
        "../third_party/walnut-cgb/minigb_apu/minigb_apu.c"
    INCLUDE_DIRS "."
                 "${CMAKE_CURRENT_LIST_DIR}/../third_party/walnut-cgb"
                 "${CMAKE_CURRENT_LIST_DIR}/../third_party/walnut-cgb/minigb_apu"
    REQUIRES kode kdrv
)

# The format macro has to be set before minigb_apu.h. Walnut is compiled into
# gbc_core.c. -fno-strict-aliasing matches the header's own note about DMA.
target_compile_definitions(${COMPONENT_LIB} PRIVATE MINIGB_APU_AUDIO_FORMAT_S16SYS)
target_compile_options(${COMPONENT_LIB} PRIVATE -fno-strict-aliasing)
