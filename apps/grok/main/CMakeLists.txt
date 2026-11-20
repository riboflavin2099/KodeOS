idf_component_register(
    SRCS
        "app_main.c"
        "app_ao.c"
        "grok_config.c"
        "grok_request.c"
        "grok_sse.c"
        "grok_transcript.c"
        "grok_prompts.c"
        "grok_http.c"
        "grok_client.c"
        "grok_net.c"
        "grok_audio.c"
        "grok_ui.c"
    INCLUDE_DIRS "."
    REQUIRES kode kode_screen esp_http_client mbedtls
)
