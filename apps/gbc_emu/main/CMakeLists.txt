idf_component_register(
    SRCS
        "app_main.c"
        "app_ao.c"
        "gbc_header.c"
        "gbc_rom.c"
        "gbc_save.c"
        "gbc_input.c"
        "gbc_audio.c"
        "gbc_display.c"
        "gbc_core.c"
        "gbc_frame.c"
    INCLUDE_DIRS "."
    REQUIRES kode kdrv
)

# Walnut-CGB (third_party/walnut-cgb/walnut_cgb.h) and minigb_apu are vendored
# and deliberately not listed here. gbc_core.c is the placeholder. Adding the
# header to a source file compiles the whole core into that file.
