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
    INCLUDE_DIRS "."
    REQUIRES kode kdrv
)

# Chocolate Doom is pinned in third_party/chocolate-doom/UPSTREAM.md and is
# deliberately not listed here. doom_engine.c is the placeholder.
